local sfl4j = require("sfl4j")

local fields = {
    {"Level", "ERROR"},
    {"Timestamp", "2014-11-21 16:35:59,501"},
    {"Class", "com.domain.client.jobs.OutgoingQueue"},
    {"ErrorMessage", "Error handling output file with job job-name"}
}

local function single_logevent()
    local log = 'ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name\n'
    local fields = sfl4j.logevent_grammar:match(log)
    if not fields then error("match didn't work " .. log) end
    assert(fields.Level == "ERROR", fields.Level)
    assert(fields.Timestamp == "2014-11-21 16:35:59,501", fields.Timestamp)
    assert(fields.Class == "com.domain.client.jobs.OutgoingQueue", fields.Class)
    assert(fields.ErrorMessage == "Error handling output file with job job-name", fields.ErrorMessage)
end

function process()
    single_logevent()

    return 0
end
