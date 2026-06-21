-- ============================================================================
-- BookofKnowledgeUI.lua (3.3.5 WotLK client)
-- ============================================================================

-- ============================================================================
-- 1. STATE VARIABLES
-- ============================================================================

local CurrentBanked = 0
local StatRows = {}
local StagedPoints = {}

local StatNames = {
    [1] = "Block",        [2] = "Dodge",       [3] = "Parry",
    [4] = "Haste",        [5] = "Hit",         [6] = "Crit",
    [7] = "Expertise",    [8] = "Attack Power", [9] = "Defense",
    [10] = "Strength",    [11] = "Stamina",    [12] = "Intellect",
    [13] = "Agility",     [14] = "Spell Power", [15] = "Spell Hit",
    [16] = "Spell Crit",  [17] = "Spell Haste"
}

-- ============================================================================
-- 2. MAIN FRAME
-- ============================================================================

local UI = CreateFrame("Frame", "BookofKnowledge", UIParent)
UI:SetSize(380, 480)
UI:SetPoint("CENTER")
UI:SetMovable(true)
UI:EnableMouse(true)
UI:RegisterForDrag("LeftButton")
UI:SetScript("OnDragStart", UI.StartMoving)
UI:SetScript("OnDragStop", UI.StopMovingOrSizing)
UI:Hide()

-- Standard WoW dialog box look
UI:SetBackdrop({
    bgFile   = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile     = true,
    tileSize = 32,
    edgeSize = 32,
    insets   = { left = 11, right = 12, top = 12, bottom = 11 }
})
UI:SetBackdropColor(1, 1, 1, 1)

-- Header
UI.Title = UI:CreateFontString(nil, "OVERLAY", "GameFontHighlightLarge")
UI.Title:SetPoint("TOP", 0, -15)
UI.Title:SetText("Book of Knowledge Allocation")

-- Close button
UI.CloseBtn = CreateFrame("Button", nil, UI, "UIPanelCloseButton")
UI.CloseBtn:SetPoint("TOPRIGHT", -5, -5)
UI.CloseBtn:SetScript("OnClick", function() UI:Hide() end)

-- Available points label
UI.BankedText = UI:CreateFontString(nil, "OVERLAY", "GameFontGreenLarge")
UI.BankedText:SetPoint("TOP", 0, -45)
UI.BankedText:SetText("Available Points: 0")

-- ============================================================================
-- 3. BOTTOM BUTTONS
-- ============================================================================

UI.TurnInBtn = CreateFrame("Button", nil, UI, "UIPanelButtonTemplate")
UI.TurnInBtn:SetSize(140, 25)
UI.TurnInBtn:SetPoint("BOTTOMLEFT", 20, 20)
UI.TurnInBtn:SetText("Turn In Books: 0")

UI.ApplyBtn = CreateFrame("Button", nil, UI, "UIPanelButtonTemplate")
UI.ApplyBtn:SetSize(140, 25)
UI.ApplyBtn:SetPoint("BOTTOMRIGHT", -20, 20)
UI.ApplyBtn:SetText("Apply Points")

-- ============================================================================
-- 4. STATE UPDATE
-- ============================================================================

local function UpdateUIState()
    UI.BankedText:SetText("Available Points: " .. CurrentBanked)

    local hasStagedPoints = false
    for statId, row in pairs(StatRows) do
        local staged = StagedPoints[statId] or 0
        local total  = row.Invested + staged

        if staged > 0 then
            hasStagedPoints = true
        end

        if total > 0 then
            row.Text:SetText(StatNames[statId] .. "    |cFF00FF00" .. total .. "|r/" .. row.Max)
        else
            row.Text:SetText(StatNames[statId] .. "    " .. total .. "/" .. row.Max)
        end

        if total >= row.Max or CurrentBanked == 0 then
            row.PlusBtn:Disable()
        else
            row.PlusBtn:Enable()
        end

        if staged > 0 then
            row.MinusBtn:Enable()
        else
            row.MinusBtn:Disable()
        end
    end

    if hasStagedPoints then
        UI.ApplyBtn:Enable()
    else
        UI.ApplyBtn:Disable()
    end
end

-- ============================================================================
-- 5. BUTTON HANDLERS
-- ============================================================================

UI.TurnInBtn:SetScript("OnClick", function()
    local text  = UI.TurnInBtn:GetText() or ""
    local books = tonumber(string.match(text, "%d+"))
    if books and books > 0 then
        CurrentBanked = CurrentBanked + books
        UI.TurnInBtn:SetText("Turn In Books: 0")
        UI.TurnInBtn:Disable()
        UpdateUIState()
        SendChatMessage("BOK_CMD:TURN_IN", "WHISPER", nil, UnitName("player"))
    end
end)

UI.ApplyBtn:SetScript("OnClick", function()
    local hasPoints = false
    for statId, amount in pairs(StagedPoints) do
        if amount > 0 then hasPoints = true; break end
    end
    if not hasPoints then return end

    for statId, amount in pairs(StagedPoints) do
        if amount > 0 and StatRows[statId] then
            StatRows[statId].Invested = StatRows[statId].Invested + amount
            SendChatMessage("BOK_CMD:APPLY:" .. statId .. ":" .. amount, "WHISPER", nil, UnitName("player"))
        end
    end

    StagedPoints = {}
    UpdateUIState()
end)

-- ============================================================================
-- 6. STAT ROW BUILDER
-- ============================================================================

UI.StatContainer = CreateFrame("Frame", nil, UI)
UI.StatContainer:SetSize(290, 380)
UI.StatContainer:SetPoint("TOP", 0, -80)

local function CreateStatRow(statId, invested, maxVal, index)
    if not StatRows[statId] then
        local row = CreateFrame("Frame", nil, UI.StatContainer)
        row:SetSize(300, 20)
        row:SetPoint("TOP", 0, -(index * 22))

        row.Text = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
        row.Text:SetPoint("LEFT", 10, 0)

        row.PlusBtn = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
        row.PlusBtn:SetSize(20, 20)
        row.PlusBtn:SetPoint("RIGHT", 0, 0)
        row.PlusBtn:SetText("+")
        row.PlusBtn:SetScript("OnClick", function()
            local currentTotal = row.Invested + (StagedPoints[statId] or 0)
            if CurrentBanked > 0 and currentTotal < row.Max then
                CurrentBanked = CurrentBanked - 1
                StagedPoints[statId] = (StagedPoints[statId] or 0) + 1
                UpdateUIState()
            end
        end)

        row.MinusBtn = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
        row.MinusBtn:SetSize(20, 20)
        row.MinusBtn:SetPoint("RIGHT", -25, 0)
        row.MinusBtn:SetText("-")
        row.MinusBtn:SetScript("OnClick", function()
            if (StagedPoints[statId] or 0) > 0 then
                CurrentBanked = CurrentBanked + 1
                StagedPoints[statId] = StagedPoints[statId] - 1
                UpdateUIState()
            end
        end)

        StatRows[statId] = row
    end

    StatRows[statId].Invested = tonumber(invested)
    StatRows[statId].Max      = tonumber(maxVal)
    StagedPoints[statId]      = 0
    StatRows[statId]:Show()
end

-- ============================================================================
-- 7. SYNC DATA PARSER
-- ============================================================================

function UI:ParseSyncData(msg)
    local parts = { strsplit(":", msg) }

    CurrentBanked = tonumber(parts[2]) or 0

    local booksInBags = tonumber(parts[3]) or 0
    UI.TurnInBtn:SetText("Turn In Books: " .. booksInBags)
    if booksInBags > 0 then
        UI.TurnInBtn:Enable()
    else
        UI.TurnInBtn:Disable()
    end

    for _, row in pairs(StatRows) do row:Hide() end
    StagedPoints = {}

    local index = 0
    for i = 4, #parts, 4 do
        local id       = tonumber(parts[i])
        local enabled  = tonumber(parts[i + 1])
        local invested = tonumber(parts[i + 2])
        local maxVal   = tonumber(parts[i + 3])

        if enabled == 1 and id then
            CreateStatRow(id, invested, maxVal, index)
            index = index + 1
        end
    end

    -- Resize frame to fit exactly the number of enabled stats
    local rowHeight   = 22
    local fixedHeight = 160
    local minHeight   = 280
    UI:SetSize(380, math.max(minHeight, fixedHeight + (index * rowHeight)))

    UpdateUIState()
    self:Show()
end

-- ============================================================================
-- 8. CHAT FILTERS
-- ============================================================================

ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(self, event, msg)
    if msg:find("%[BOK_SYNC%]") then
        UI:ParseSyncData(msg)
        return true
    end
    return false
end)

ChatFrame_AddMessageEventFilter("CHAT_MSG_WHISPER", function(self, event, msg)
    if msg:find("BOK_CMD:") then return true end
    return false
end)

ChatFrame_AddMessageEventFilter("CHAT_MSG_WHISPER_INFORM", function(self, event, msg)
    if msg:find("BOK_CMD:") then return true end
    return false
end)