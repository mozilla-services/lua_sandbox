local sfl4j = require("sfl4j")

local single_line_log = 'ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name\n'
local single_line_test_fields = {
    Level        = "ERROR",
    Timestamp    = "2014-11-21 16:35:59,501",
    Class        = "com.domain.client.jobs.OutgoingQueue",
    ErrorMessage = "Error handling output file with job job-name"
}

local function single_logevent()
    local fields = sfl4j.logevent_grammar:match(single_line_log)
    if not fields then error("match didn't work " .. single_line_log) end
    assert(fields.Level == single_line_test_fields.Level, fields.Level)
    assert(fields.Timestamp == single_line_test_fields.Timestamp, fields.Timestamp)
    assert(fields.Class == single_line_test_fields.Class, fields.Class)
    assert(fields.ErrorMessage == single_line_test_fields.ErrorMessage, fields.ErrorMessage)
end

local function mult_line_logevent()
end

function process()
    single_logevent()

    return 0
end
