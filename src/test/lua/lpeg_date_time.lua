-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"

local dt = require("date_time")

local function test_valid(grammar, tests, results)
    for i,v in ipairs(tests) do
        t = grammar:match(v)
        if not t then
            error(v)
        else
            ns = dt.time_to_ns(t)
            if ns ~= results[i] then
                error(string.format("test: %d %s expected: %g received: %g", i, v, results[i], ns), 2)
            end
        end
    end
end

local function rfc3339()
    local tests = {"1999-05-05T23:23:59.217-07:00",
        "1985-04-12T23:20:50.52Z",
        "1996-12-19T16:39:57-08:00",
        "1990-12-31T23:59:60Z",
        "1990-12-31T15:59:60-08:00",
        "1937-01-01T12:00:27.87+00:20"}
    local results = {925971839217000064,
        482196050520000000,
        851042397000000000,
        662688000000000000,
        662688000000000000,
        -1041337172130000000}
    if os.date("%c"):find("^%d") then
        results[6] = 0 -- windows will fail to convert this time
    end
    test_valid(dt.rfc3339, tests, results)
end

local function rfc3339_invalid()
    local tests = {"1985-04-12t23:20:50.52Z",
        "1985-04-12T23:20:50.52z",
        "1985-04-12",
        "1999-05-05T23:23:59.217-0700"}
    for i,v in ipairs(tests) do
        if dt.rfc3339:match(v) then
            error(v)
        end
    end
end

local function clf()
    local tests = {"10/Feb/2014:08:46:36 -0800"}
    local results = {1392050796000000000}
    test_valid(dt.clf_timestamp, tests, results)
end

local function rfc3164()
    local tests = {"Feb 10 16:46:36"}
    local results = {os.time{year = os.date("%Y"), month = 2, day = 10, hour = 16, min = 46, sec = 36} * 1e9}
    test_valid(dt.rfc3164_timestamp, tests, results)
end

local function mysql()
    local tests = {"20140210164636"}
    local results = {1392050796000000000}
    test_valid(dt.mysql_timestamp, tests, results)
end

local function pgsql()
    local tests = {"2014-02-10 16:46:36"}
    local results = {1392050796000000000}
    test_valid(dt.pgsql_timestamp, tests, results)
end

local function strftime_all()
    local formats
    if os.date("%c"):find("^%d") then -- windows C89 support only
        formats = {"%a", "%A", "%b", "%B","%c", "%d",
             "%H", "%I", "%j", "%m", "%M", "%p",
             "%S", "%U", "%w", "%W", "%x",
             "%X", "%y", "%Y", "%z", "%Z", "%%", "test string"}
    else
        formats = {"%a", "%A", "%b", "%B","%c","%C", "%d", "%D", "%e",
            "%F", "%g", "%G", "%h", "%H", "%I", "%j", "%k", "%l", "%m",
            "%M", "%n", "%p", "%r", "%R", "%s", "%S", "%t", "%T", "%u",
            "%U", "%V", "%w", "%W", "%x","%X", "%y", "%Y", "%z", "%Z",
            "%%", "test string"}
    end

    for i,v in ipairs(formats) do
        local test = os.date(v)
        local g = dt.build_strftime_grammar(v)
        if not g:match(test) then
            error(string.format("failed parsing: %s %s", v, test))
        end
    end
end

local function strftime_composite()
    local formats = {"%c","%D %T","%D %r","%d/%b/%Y:%H:%M:%S %z"}
    local inputs = {"Mon Feb 10 16:46:36 2014", "02/10/14 16:46:36", "02/10/14 04:46:36 PM", "10/Feb/2014:16:46:36 +0000"}
    if os.date("%c"):find("^%d") then -- windows %c is non-standard
        inputs[1] = inputs[2]
    end
    local result = 1392050796000000000
    for i,v in ipairs(formats) do
        local g = dt.build_strftime_grammar(v)
        local t = g:match(inputs[i])
        if not t then
            error(string.format("failed parsing: %s %s", v, inputs[i]))
        end
        if result ~=  dt.time_to_ns(t) then
            error(string.format("time conversion failed %s", inputs[i]))
        end
    end
end

local function strftime_invalid()
    local r, g = pcall(dt.build_strftime_grammar, "%E")
    if r then
        error("allowed to build a grammar with an invalid specifier")
    end
end

function process()
    rfc3339()
    rfc3339_invalid()
    clf()
    rfc3164()
    mysql()
    pgsql()
    strftime_all()
    strftime_composite()
    strftime_invalid()

    return 0
end
