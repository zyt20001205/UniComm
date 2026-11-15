-- Configure a Vision port named "Vision" and its ROIs/pipeline in port settings.
local vision = Port.get("Vision")
vision:open()

-- Always returns an array, with one structured recognition result per ROI.
local results = vision:read(0, 20000)
---@cast results VisionResult[]
for index, result in ipairs(results) do
    print("ROI:", index, result.width, result.height)
    print("Text:", result.text)
    for _, item in ipairs(result.items) do
        print(item.x, item.y, item.width, item.height, item.text, item.confidence)
    end
end

-- Export the complete source frame to a workspace-relative path.
vision:write([[raw "raw.png"]])

-- ROI indexes start at 1. Configure an ROI before using these commands.
-- vision:write([[roi 1 "roi-1.png"]])
-- vision:write([[processed 1 "processed-1.png"]])

-- Export raw, ROIs, processed frames and result.json together.
-- vision:write([[bundle "capture"]])
vision:close()
