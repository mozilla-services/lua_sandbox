
-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "lpeg"

local ip = require "lpeg.ip_address"

local function ipv6()
    local tests = {"2001:0db8:85a3:0000:0000:8a2e:0370:7334",
        "2001:db8:85a3:0:0:8a2e:370:7334",
        "2001:db8:85a3::8a2e:370:7334",
        "2001:0db8:0000:0000:0000:0000:1428:57ab",
        "2001:0db8:0000:0000:0000::1428:57ab",
        "2001:0db8:0:0:0:0:1428:57ab",
        "2001:0db8:0:0::1428:57ab",
        "2001:0db8::1428:57ab",
        "2001:db8::1428:57ab",
        "0000:0000:0000:0000:0000:0000:0000:0001",
        "::1",
        "::ffff:12.34.56.78",
        "::ffff:0c22:384e",
        "2001:0db8:1234:0000:0000:0000:0000:0000",
        "2001:0db8:1234:ffff:ffff:ffff:ffff:ffff",
        "2001:db8:a::123",
        "fe80::",
        "::ffff:192.0.2.128",
        "::ffff:c000:280",
        "::"}
    for i,v in ipairs(tests) do
        if not ip.v6:match(v) then
            error(v)
        end
    end
end

local function ipv6_invalid()
    local tests = {
        "123",
        "ldkfj",
        "2001::FFD3::57ab",
        "2001:db8:85a3::8a2e:37023:7334",
        "2001:db8:85a3::8a2e:370k:7334",
        "1:2:3:4:5:6:7:8:9",
        "1::2::3",
        "1:::3:4:5",
        "1:2:3::4:5:6:7:8:9",
        "::ffff:2.3.4",
        "::ffff:257.1.2.3",
        "1.2.3.4"}
    local grammar = ip.v6 * lpeg.P(-1) -- make it a full match
    for i,v in ipairs(tests) do
        if grammar:match(v) then
            error(v)
        end
    end
end

local function ipv4()
    local tests = {
        "0.0.0.0",
        "192.168.1.10",
        "127.0.0.1",
        "249.255.255.255",
        "255.255.255.255",
        "1.2.3.4"}
    for i,v in ipairs(tests) do
        if not ip.v4:match(v) then
            error(v)
        end
    end
end

local function ipv4_invalid()
    local grammar = ip.v4 * lpeg.P(-1) -- make it a full match
    local tests = {
        "00.0.0.0",
        "256.255.255.255",
        "255.2550.255.255",
        "1",
        "1.2",
        "1.2.3.",
        "1.2.3.a",
        "1.2.3.4.5",
        "."}
    for i,v in ipairs(tests) do
        if grammar:match(v) then
            error(v)
        end
    end
end

local function hostname()
    local tests = {
        "localhost",
        "example.com",
        "127.org",
        "example%20",
        "UPPERCASE.COM",
        "aA1-._~!$&'()*+,;=.com"}
    for i,v in ipairs(tests) do
        if not ip.hostname:match(v) then
            error(v)
        end
    end
end

local function hostname_invalid()
    local grammar = ip.hostname * lpeg.P(-1) -- make it a full match
    local tests = {
        "example com",
        "foo%AG",
        "at@",
        "colon:",
        "lbracket[",
        "rbracket]",
        "hash#",
        "quest?",
        "slash/",
        "lbrace{",
        "rbrace}",
        ""}
    for i,v in ipairs(tests) do
        if grammar:match(v) then
            error(v)
        end
    end
end

local function host()
    local tests = {
        "localhost",
        "127.0.0.1",
        "::1"}
    for i,v in ipairs(tests) do
        if not ip.host:match(v) then
            error(v)
        end
    end
end

local function host_field()
    local tests = {
        {"localhost", "hostname"},
        {"127.0.0.1", "ipv4"},
        {"::1", "ipv6"}
    }
    for i,v in ipairs(tests) do
        local field = ip.host_field:match(v[1])
        if field then
            assert(field.value == tests[i][1], field.value)
            assert(field.representation == tests[i][2], field.representation)
        else
            error(v)
        end
    end
end

function process(tc)
    ipv6()
    ipv6_invalid()
    ipv4()
    ipv4_invalid()
    hostname()
    hostname_invalid()
    host()
    host_field()
    return 0
end
