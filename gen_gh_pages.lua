-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "os"
require "io"
require "string"


local function output_menu(output_dir, version)
    local fh = assert(io.open(string.format("%s/SUMMARY.md", output_dir), "w"))
    fh:write(string.format("* [Lua Sandbox Library (%s)](README.md)\n\n", version))
    fh:write([[
* [Generic Sandbox](sandbox.md)
* [Heka Sandbox](heka/index.md)
   * [Input Interface](heka/input.md)
   * [Analysis Interface](heka/analysis.md)
   * [Output Interface](heka/output.md)
   * [Message Schema](heka/message.md)
   * [Message Matcher](util/message_matcher.md)
* [Command Line Tools](cli/index.md)
* [Source Documentation](https://mozilla-services.github.io/lua_sandbox/doxygen/index.html)
* [Sandbox Extensions](https://mozilla-services.github.io/lua_sandbox_extensions)
]])
    fh:close()
end


local args = {...}
local function main()
    local output_dir = string.format("%s/gb-source", arg[3])
    local rv = os.execute(string.format("rsync -rav docs/ %s/", output_dir))
    if rv ~= 0 then error"rsync setup" end

    local fh = assert(io.open(string.format("%s/book.json", output_dir), "w"))
    fh:write([[{"plugins" : ["collapsible-menu", "navigator"]}]])
    fh:close()

    os.execute(string.format("cd %s;gitbook install", output_dir))
    os.execute(string.format("mv %s/index.md %s/README.md", output_dir, output_dir))
    output_menu(output_dir, args[1])
    os.execute(string.format("gitbook build %s", output_dir))
    local rv = os.execute(string.format("rsync -rav %s/_book/ %s/", output_dir, "gh-pages/"))
    if rv ~= 0 then error"rsync publish" end
end

main()
