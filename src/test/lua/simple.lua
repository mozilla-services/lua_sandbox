gint = 1

function process(tc)
    if tc == 1 then
        return 0, "ok"
    elseif tc == 2  then
        return 0, true
    end
    return 0
end

function report()
end
