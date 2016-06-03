-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "os"
require "io"
require "string"
require "table"

local function get_path(s)
    return s:match("(.+)/[^/]-$")
end


local function get_filename(s)
    return s:match("/([^/]-)$")
end


local function strip_ext(s)
    return s:sub(1, #s - 4)
end


local function sort_entries(t)
  local a = {}
  for n in pairs(t) do table.insert(a, n) end
  table.sort(a)
  local i = 0      -- iterator variable
  local iter = function ()   -- iterator function
    i = i + 1
    if a[i] == nil then
        return nil
    else
        return a[i], t[a[i]]
    end
  end
  return iter
end


local function create_index(path, dir)
    local fh = assert(io.open("gh-pages/" .. path .. "/index.md", "w"))
    fh:write(string.format("# %s\n", path))

    for k,v in sort_entries(dir.entries) do
        if k:match(".lua$") then
            fh:write(string.format("* [%s](%s.html) - %s\n", k, strip_ext(k), v.title))
        else
            fh:write(string.format("* [%s](%s/index.html)\n", k, k))
        end
    end

    fh:close()
end


local function output_tree(fh, list, key, dir)
    list[#list + 1] = key
    local path = table.concat(list, "/")
    create_index(path, dir)
    if #list == 1 then fh:write("<ul>\n") end
    fh:write(string.format('<li><a href="/lua_sandbox/%s/index.html">%s</a></li>\n', path, key))

    fh:write("<ul>\n")
    for k, v in sort_entries(dir.entries) do
        if k:match(".lua$") then
            fh:write(string.format('<li><a href="/lua_sandbox/%s.html">%s</a></li>\n', strip_ext(v.line), strip_ext(k)))
        else
            output_tree(fh, list, k, v)
        end
    end
    fh:write("</ul>\n")

    if #list == 1 then fh:write("</ul>\n") end
    table.remove(list)
end


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
            <li>C Modules</li>
            <ul>
                <li><a href="https://github.com/mozilla-services/lua_bloom_filter/blob/master/README.md">bloom_filter</a></li>
                <li><a href="https://github.com/mozilla-services/lua_circular_buffer/blob/master/README.md">circular_buffer</a></li>
                <li><a href="http://www.kyne.com.au/~mark/software/lua-cjson-manual.html">cjson</a></li>
                <li><a href="https://github.com/mozilla-services/lua_cuckoo_filter/blob/master/README.md">cuckoo_filter</a></li>
                <li><a href="https://github.com/mozilla-services/lua_hyperloglog/blob/master/README.md">hyperloglog</a></li>
                <li><a href="http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html">lpeg</a></li>
                <li><a href="http://www.inf.puc-rio.br/~roberto/lpeg/re.html">re</a></li>
                <li><a href="https://github.com/trink/symtseries/blob/master/README.md">sax</a></li>
                <li><a href="http://www.inf.puc-rio.br/~roberto/struct/">struct</a></li>
            </ul>
        </ul>
        ]])

    output_tree(fh, {}, "modules", paths.entries.modules)
    output_tree(fh, {}, "sandboxes", paths.entries.sandboxes)

    fh:write([[
    </div>
    <div class="main-content">
]])
    fh:close()

    fh = assert(io.open(after, "w"))
    fh:write("</div>\n")
    fh:close()
end


local function handle_path(paths, in_path, out_path)
    local list = {}
    local d = paths
    for dir in string.gmatch(out_path, "[^/]+") do
        list[#list + 1] = dir
        if dir ~= "gh-pages" then
            local full_path = table.concat(list, "/")
            local cd = d.entries[dir]
            if not cd then
                os.execute(string.format("mkdir -p %s", full_path))
                local nd = {path = in_path, entries = {}}
                d.entries[dir] = nd
                d = nd
            else
                d = cd
            end
        end
    end
    return d
end


local function extract_lua_docs(paths)
    local fh = assert(io.popen("find sandboxes modules -name \\*.lua"))
    for line in fh:lines() do
        local sfh = assert(io.open(line))
        local lua = sfh:read("*a")
        sfh:close()

        local doc = lua:match("%-%-%[%[%s*(.-)%-%-%]%]")
        local title = lua:match("#%s(.-)\n")
        if not title then error("doc error, no title: " .. line) end

        local outfn = string.gsub("gh-pages/" .. line, "lua$", "md")
        local p = handle_path(paths, get_path(line), get_path(outfn))
        local ofh = assert(io.open(outfn, "w"))
        p.entries[get_filename(line)] = {line = line, title = title}
        ofh:write(doc)
        ofh:write(string.format("\n\nsource code: [%s](https://github.com/mozilla-services/lua_sandbox/blob/master/%s)\n", get_filename(line), line))
        ofh:close()
    end
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
    extract_lua_docs(paths)
    md_to_html(paths, args[1])
end

main()
