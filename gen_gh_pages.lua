-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "os"
require "io"
require "string"

local function output_css()
    local fh = assert(io.open("gh-pages/docs.css", "w"))
    fh:write([[
    html {
        height: 100%;
    }

    body {
        font-family:verdana, arial, sans-serif;
        font-size:small;
        width: 90%;
        background:white;
        margin-left: auto;
        margin-right: auto;
        height: 100%;
    }

    h1 {
        border-bottom:1px black solid;
    }

    h2 {
        border-bottom:1px gray solid;
    }

    h3 {
        border-bottom:1px lightgray solid;
    }

    h4 {
        border-bottom:1px black dotted;
    }

    h5 {
        border-bottom:1px gray dotted;
    }

    h6 {
        border-bottom:1px lightgray dotted;
    }

    #title {
        width:100%;
        font-size:large;
        font-weight: bold;
        font-style: normal;
        font-variant: normal;
        text-transform: uppercase;
        letter-spacing: .1em;
    }

    .menu {
        display:table-cell;
        font-size: small;
        font-weight: normal;
        font-style: normal;
        font-variant:  small-caps;
        color: #000000;
        height: 100%;
        padding-right: 10px;
        white-space: nowrap;
    }

    .menu ul{
        list-style-type: none;
        margin-left: 5px;
        margin-right: 0px;
        padding-left: 10px;
        padding-right: 0px;
    }

    .main-content {
        border-left:1px lightgray dotted;
        padding-left:10px;
        display:table-cell;
        width:100%;
    }

    code, pre.code, pre.sourceCode
    {
        background-color: whitesmoke;
    }
    ]])
    fh:close()
end


local function output_menu(before, after, paths, version)
    local fh = assert(io.open(before, "w"))
    fh:write(string.format('<div id="title">Lua Sandbox Library (%s)<hr/></div>\n', version))
    fh:write([[<div class="menu">
        <ul>
            <li><a href="/lua_sandbox/index.html">Overview</a></li>
        </ul>
        <ul>
            <li><a href="/lua_sandbox/sandbox.html">Generic Sandbox</a></li>
        </ul>
        <ul>
            <li><a href="/lua_sandbox/heka/index.html">Heka Sandbox</a></li>
        <ul>
            <li><a href="/lua_sandbox/heka/analysis.html">Analysis Interface</a></li>
            <li><a href="/lua_sandbox/heka/input.html">Input Interface</a></li>
            <li><a href="/lua_sandbox/heka/output.html">Output Interface</a></li>
        </ul>
        </ul>
        <ul>
           <li><a href="/lua_sandbox/cli/index.html">Command Line Tools</a></li>
        </ul>
        <ul>
            <li><a href="/lua_sandbox/doxygen/index.html">Source Documentation</a></li>
        </ul>
        <ul>
            <li><a href="/lua_sandbox_extensions/index.html">Sandbox Extensions</a></li>
        </ul>
    </div>
    <div class="main-content">
]])
    fh:close()

    fh = assert(io.open(after, "w"))
    fh:write("</div>\n")
    fh:close()
end


local function md_to_html(paths, version)
    local before = "/tmp/before.html"
    local after = "/tmp/after.html"
    output_menu(before, after, paths, version)

    local fh = assert(io.popen("find gh-pages -name \\*.md"))
    for line in fh:lines() do
        local css_path = "/lua_sandbox/docs.css"
        local cmd = string.format("pandoc --from markdown_github-hard_line_breaks --to html --standalone -B %s -A %s -c %s -o %s.html %s", before, after, css_path, line:sub(1, #line -3), line)
        local rv = os.execute(cmd)
        if rv ~= 0 then error(cmd) end
        os.remove(line)
    end
    fh:close()

    os.remove(before)
    os.remove(after)
end


local args = {...}
local function main()
    local rv = os.execute("rsync -rav docs/ gh-pages/")
    if rv ~= 0 then error"rsync" end
    output_css()
    local paths = {entries = {}}
    md_to_html(paths, args[1])
end

main()
