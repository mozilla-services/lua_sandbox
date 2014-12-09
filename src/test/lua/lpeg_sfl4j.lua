local sfl4j = require("sfl4j")

local test_log_events = {
[[
ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name
]]
}

local fields = {
    {"Level", "ERROR"},
    {"Timestamp", ""},
    {"Class", "com.domain.client.jobs.OutgoingQueue"},
    {"ErrorMessage", "Error handling output file with job job-name"}
}

local function validate(fields, t)
    for i, v in ipairs(fields) do
    end
    return false
end

local function mainlogevent()
    local t = sfl4j.logevent_grammar:match(test_log_events[1])
    validate(fields, t)
end

function process(tc)
    mainlogevent()

    return 1
end
