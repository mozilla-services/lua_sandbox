local sfl4j = require("sfl4j")

local fields = {
    {"Level", "ERROR"},
    {"Timestamp", ""},
    {"Class", "com.domain.client.jobs.OutgoingQueue"},
    {"ErrorMessage", "Error handling output file with job job-name"}
}

local function single_logevent()
    local log = 'ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name'
    local fields = slf4j.logevent_grammar:match(log)
    assert(fields.Level == "ERROR", fields.Level)
end

function process()
    single_logevent()

    return 0
end
