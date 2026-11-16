#include "document/module/webviewWidget.h"

#include <QColor>
#include <QDir>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

#include <atomic>
#include <functional>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WebView2.h>

#include "globals.h"
#include "core/globalManager.h"

namespace {
    template<typename Interface>
    class CallbackBase : public Interface {
    public:
        explicit CallbackBase(const IID &interfaceId)
            : m_interfaceId(interfaceId) {
        }

        ULONG STDMETHODCALLTYPE AddRef() override {
            return ++m_ref;
        }

        ULONG STDMETHODCALLTYPE Release() override {
            const auto ref = --m_ref;
            if (ref == 0) delete this;
            return ref;
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **object) override {
            if (!object) return E_POINTER;
            if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, m_interfaceId)) {
                *object = static_cast<Interface *>(this);
                AddRef();
                return S_OK;
            }
            *object = nullptr;
            return E_NOINTERFACE;
        }

    private:
        IID m_interfaceId{};
        std::atomic<ULONG> m_ref{1};
    };

    class EnvironmentCompletedHandler final : public CallbackBase<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
    public:
        explicit EnvironmentCompletedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Environment *)> callback)
            : CallbackBase(IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler),
              m_callback(std::move(callback)) {
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment *environment) override {
            return m_callback(result, environment);
        }

    private:
        std::function<HRESULT(HRESULT, ICoreWebView2Environment *)> m_callback;
    };

    class ControllerCompletedHandler final : public CallbackBase<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
    public:
        explicit ControllerCompletedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Controller *)> callback)
            : CallbackBase(IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler),
              m_callback(std::move(callback)) {
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller *controller) override {
            return m_callback(result, controller);
        }

    private:
        std::function<HRESULT(HRESULT, ICoreWebView2Controller *)> m_callback;
    };

    class NavigationCompletedHandler final : public CallbackBase<ICoreWebView2NavigationCompletedEventHandler> {
    public:
        explicit NavigationCompletedHandler(std::function<HRESULT(ICoreWebView2NavigationCompletedEventArgs *)> callback)
            : CallbackBase(IID_ICoreWebView2NavigationCompletedEventHandler),
              m_callback(std::move(callback)) {
        }

        HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *args) override {
            return m_callback(args);
        }

    private:
        std::function<HRESULT(ICoreWebView2NavigationCompletedEventArgs *)> m_callback;
    };

    class NavigationStartingHandler final : public CallbackBase<ICoreWebView2NavigationStartingEventHandler> {
    public:
        explicit NavigationStartingHandler(std::function<HRESULT(ICoreWebView2NavigationStartingEventArgs *)> callback)
            : CallbackBase(IID_ICoreWebView2NavigationStartingEventHandler),
              m_callback(std::move(callback)) {
        }

        HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args) override {
            return m_callback(args);
        }

    private:
        std::function<HRESULT(ICoreWebView2NavigationStartingEventArgs *)> m_callback;
    };

    class NewWindowRequestedHandler final : public CallbackBase<ICoreWebView2NewWindowRequestedEventHandler> {
    public:
        explicit NewWindowRequestedHandler(std::function<HRESULT(ICoreWebView2NewWindowRequestedEventArgs *)> callback)
            : CallbackBase(IID_ICoreWebView2NewWindowRequestedEventHandler),
              m_callback(std::move(callback)) {
        }

        HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *args) override {
            return m_callback(args);
        }

    private:
        std::function<HRESULT(ICoreWebView2NewWindowRequestedEventArgs *)> m_callback;
    };
} // namespace

WebviewWidget::WebviewWidget(QWidget *parent)
    : QWidget(parent),
      m_host(new QWidget(this)) {
    // Isolate the native HWND so Qt does not make the surrounding splitter's children native.
    m_host->setAttribute(Qt::WA_DontCreateNativeAncestors);
    m_host->setAttribute(Qt::WA_NativeWindow);
    m_host->installEventFilter(this);
    auto *layout = new QVBoxLayout(this); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_host);
    setFocusPolicy(Qt::StrongFocus);

    const auto hr = OleInitialize(nullptr);
    m_oleInitialized = SUCCEEDED(hr);
    if (FAILED(hr)) emit errorOccurred("Failed to initialize COM");
}

WebviewWidget::~WebviewWidget() {
    closeWebView();
    if (m_oleInitialized) OleUninitialize();
}

void WebviewWidget::navigate(const QUrl &url) {
    m_pendingHtml.clear();
    m_pendingUrl = url.toString();
    ensureCreated();
    if (!m_webView) return;

    const auto hr = m_webView->Navigate(m_pendingUrl.toStdWString().c_str());
    if (FAILED(hr)) emit errorOccurred("Failed to navigate");
}

void WebviewWidget::setHtml(const QString &html) {
    m_pendingUrl.clear();
    m_pendingHtml = html;
    ensureCreated();
    if (!m_webView) return;

    const auto hr = m_webView->NavigateToString(m_pendingHtml.toStdWString().c_str());
    if (FAILED(hr)) emit errorOccurred("Failed to render HTML");
}

void WebviewWidget::reload() const {
    if (m_webView) m_webView->Reload();
}

void WebviewWidget::stop() const {
    if (m_webView) m_webView->Stop();
}

bool WebviewWidget::isReady() const {
    return m_ready;
}

bool WebviewWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_host && event->type() == QEvent::Resize) resizeWebView();
    if (watched == m_host && m_controller && (event->type() == QEvent::Show || event->type() == QEvent::Hide)) {
        m_controller->put_IsVisible(m_host->isVisible());
    }
    return QWidget::eventFilter(watched, event);
}

void WebviewWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this] { ensureCreated(); });
}

void WebviewWidget::ensureCreated() {
    if (m_ready || m_creating || !m_oleInitialized || !isVisible()) return;

    m_creating = true;
    const auto userData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/webview2";
    if (!QDir().mkpath(userData)) return;

    const QPointer<WebviewWidget> guard(this);
    auto *environmentHandler = new EnvironmentCompletedHandler([this, guard](const HRESULT result, ICoreWebView2Environment *createdEnvironment) -> HRESULT {
        if (!guard) return S_OK;
        if (FAILED(result)) {
            m_creating = false;
            emit errorOccurred("Failed to create WebView2 environment");
            return S_OK;
        }

        m_environment = createdEnvironment;
        m_environment->AddRef();

        auto *controllerHandler = new ControllerCompletedHandler([this, guard](const HRESULT controllerResult, ICoreWebView2Controller *createdController) -> HRESULT {
            if (!guard) {
                if (createdController) createdController->Close();
                return S_OK;
            }
            m_creating = false;
            if (FAILED(controllerResult)) {
                emit errorOccurred("Failed to create WebView2 controller");
                return S_OK;
            }

            m_controller = createdController;
            m_controller->AddRef();

            const auto webViewHr = m_controller->get_CoreWebView2(&m_webView);
            if (FAILED(webViewHr)) {
                emit errorOccurred("Failed to get WebView2");
                return S_OK;
            }
            themeApply();
            registerEvents();

            if (SUCCEEDED(m_webView->QueryInterface(IID_ICoreWebView2_28, (void**)&m_webView28))) {
                std::wstring mermaidPath = (QCoreApplication::applicationDirPath() + "/javascript").toStdWString();
                m_webView28->SetVirtualHostNameToFolderMapping(
                    L"unicomm",
                    mermaidPath.c_str(),
                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
                );
            }

            resizeWebView();
            m_controller->put_IsVisible(m_host->isVisible());
            QTimer::singleShot(0, this, [this] {
                for (auto *ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
                    ancestor->update();
                }
            });
            m_ready = m_webView && m_controller;
            emit readyChanged(m_ready);
            if (!guard) return S_OK;
            if (!m_pendingHtml.isEmpty()) setHtml(m_pendingHtml);
            else if (!m_pendingUrl.isEmpty()) navigate(m_pendingUrl);
            return S_OK;
        });

        const auto controllerHr = m_environment->CreateCoreWebView2Controller(
            reinterpret_cast<HWND>(m_host->winId()),
            controllerHandler);
        controllerHandler->Release();

        if (FAILED(controllerHr)) {
            m_creating = false;
            emit errorOccurred("Failed to request WebView2 controller");
        }
        return S_OK;
    });

    const auto hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        userData.toStdWString().c_str(),
        nullptr,
        environmentHandler);
    environmentHandler->Release();

    if (FAILED(hr)) {
        m_creating = false;
        emit errorOccurred("Failed to request WebView2 environment");
    }
}

void WebviewWidget::resizeWebView() const {
    if (!m_controller) return;

    RECT bounds{};
    if (GetClientRect(reinterpret_cast<HWND>(m_host->winId()), &bounds)) m_controller->put_Bounds(bounds);
}

void WebviewWidget::closeWebView() {
    if (m_controller) m_controller->Close();
    if (m_webView28) {
        m_webView28->Release();
        m_webView28 = nullptr;
    }
    if (m_webView) {
        m_webView->Release();
        m_webView = nullptr;
    }
    if (m_controller) {
        m_controller->Release();
        m_controller = nullptr;
    }
    if (m_environment) {
        m_environment->Release();
        m_environment = nullptr;
    }
    m_ready = false;
    m_creating = false;
}

void WebviewWidget::themeApply() const {
    if (!g_globalManager || !m_controller || !m_webView) return;

    ICoreWebView2_13 *webView13{};
    if (SUCCEEDED(m_webView->QueryInterface(IID_ICoreWebView2_13, reinterpret_cast<void **>(&webView13)))) {
        ICoreWebView2Profile *profile{};
        if (SUCCEEDED(webView13->get_Profile(&profile))) {
            const auto colorScheme = g_globalManager->themeGet()->modeGet() == Theme::Light
                                         ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT
                                         : COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK;
            profile->put_PreferredColorScheme(colorScheme);
            profile->Release();
        }
        webView13->Release();
    }

    ICoreWebView2Controller2 *controller2{};
    if (SUCCEEDED(m_controller->QueryInterface(IID_ICoreWebView2Controller2, reinterpret_cast<void **>(&controller2)))) {
        const QColor background(g_globalManager->themeGet()->backGet());
        const COREWEBVIEW2_COLOR color{
            255,
            static_cast<BYTE>(background.red()),
            static_cast<BYTE>(background.green()),
            static_cast<BYTE>(background.blue())
        };
        controller2->put_DefaultBackgroundColor(color);
        controller2->Release();
    }
}

void WebviewWidget::registerEvents() {
    const QPointer<WebviewWidget> guard(this);
    auto *handler = new NavigationCompletedHandler([this, guard](ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
        if (!guard) return S_OK;
        BOOL success = FALSE;
        if (args) args->get_IsSuccess(&success);
        emit loadFinished(success);
        return S_OK;
    });
    m_webView->add_NavigationCompleted(handler, nullptr);
    handler->Release();

    auto *navigationHandler = new NavigationStartingHandler([this, guard](ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
        if (!guard || !m_delegateLinks) return S_OK;
        LPWSTR uri{};
        if (FAILED(args->get_Uri(&uri))) return S_OK;
        const auto uriString = QString::fromWCharArray(uri);
        const QUrl url(uriString);
        CoTaskMemFree(uri);
        if (url == QUrl("about:blank")) return S_OK;
        // allow the internal data navigation generated by NavigateToString.
        if (url.scheme() == "data" && !m_pendingHtml.isEmpty()) {
            const auto separator = uriString.indexOf(',');
            const auto metadata = uriString.left(separator);
            if (separator >= 0 && metadata.section(';', 0, 0).compare("data:text/html", Qt::CaseInsensitive) == 0) {
                auto content = QByteArray::fromPercentEncoding(uriString.mid(separator + 1).toUtf8());
                if (metadata.endsWith(";base64", Qt::CaseInsensitive)) content = QByteArray::fromBase64(content);
                if (QString::fromUtf8(content) == m_pendingHtml) return S_OK;
            }
        }
        args->put_Cancel(TRUE);
        BOOL userInitiated = FALSE;
        args->get_IsUserInitiated(&userInitiated);
        if (userInitiated) emit linkClicked(url);
        return S_OK;
    });
    m_webView->add_NavigationStarting(navigationHandler, nullptr);
    navigationHandler->Release();

    auto *windowHandler = new NewWindowRequestedHandler([this, guard](ICoreWebView2NewWindowRequestedEventArgs *args) -> HRESULT {
        if (!guard || !m_delegateLinks) return S_OK;
        args->put_Handled(TRUE);
        BOOL userInitiated = FALSE;
        args->get_IsUserInitiated(&userInitiated);
        if (!userInitiated) return S_OK;
        LPWSTR uri{};
        if (SUCCEEDED(args->get_Uri(&uri))) {
            const QUrl url(QString::fromWCharArray(uri));
            CoTaskMemFree(uri);
            emit linkClicked(url);
        }
        return S_OK;
    });
    m_webView->add_NewWindowRequested(windowHandler, nullptr);
    windowHandler->Release();
}
