local sfl4j = require("sfl4j")
local dt    = require("date_time")

-- Parse a basic message, these are typically not errors, but whatever.
local function single_line_logevent()
    local single_line_log = 'ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name\n'
    local single_line_test_fields = {
        severity  = 3,
        timestamp = 1.416587759501e+18,
        class     = "com.domain.client.jobs.OutgoingQueue",
        message   = "Error handling output file with job job-name"
    }

    local fields = sfl4j.logevent_grammar:match(single_line_log)
    if not fields then error("match didn't work " .. single_line_log) end

    assert(fields.severity == single_line_test_fields.severity, fields.severity)
    assert(fields.timestamp == single_line_test_fields.timestamp, fields.timestamp)
    assert(fields.class == single_line_test_fields.class, fields.class)
    assert(fields.message == single_line_test_fields.message, fields.message)
end

-- Use this to easily debug multi-line messages
local function two_line_logevent()
    local two_line_log = [[
ERROR [2014-11-20 16:40:51,577] com.domain.substitute.UsersLoop: Error caught in user loop
! java.net.SocketTimeoutException: Read timed out
    ]]
    local two_line_test_fields = {
        severity   = 3,
        timestamp  = 1.416501651577e+18,
        class      = "com.domain.substitute.UsersLoop",
        message    = "Error caught in user loop",
        stacktrace = "! java.net.SocketTimeoutException: Read timed out\n"
    }

    local fields = sfl4j.logevent_grammar:match(two_line_log)
    if not fields then error("match didn't work " .. two_line_log) end

    assert(fields.severity == two_line_test_fields.severity, fields.severity)
    assert(fields.timestamp == two_line_test_fields.timestamp, fields.timestamp)
    assert(fields.class == two_line_test_fields.class, fields.class)
    assert(fields.message == two_line_test_fields.message, fields.message)
    assert(fields.Stacktrace == two_line_test_fields.Stacktrace, fields.Stacktrace)
end


local multi_line_log = [[
ERROR [2014-11-20 16:40:51,577] com.domain.substitute.UsersLoop: Error caught in user loop
! java.net.SocketTimeoutException: Read timed out
! at java.net.SocketInputStream.socketRead0(Native Method)
! at java.net.SocketInputStream.read(SocketInputStream.java:152)
! at java.net.SocketInputStream.read(SocketInputStream.java:122)
! at sun.security.ssl.InputRecord.readFully(InputRecord.java:442)
! at sun.security.ssl.InputRecord.read(InputRecord.java:480)
! at sun.security.ssl.SSLSocketImpl.readRecord(SSLSocketImpl.java:927)
! at sun.security.ssl.SSLSocketImpl.readDataRecord(SSLSocketImpl.java:884)
! at sun.security.ssl.AppInputStream.read(AppInputStream.java:102)
! at org.apache.http.impl.io.SessionInputBufferImpl.streamRead(SessionInputBufferImpl.java:136)
! at org.apache.http.impl.io.SessionInputBufferImpl.fillBuffer(SessionInputBufferImpl.java:152)
! at org.apache.http.impl.io.SessionInputBufferImpl.readLine(SessionInputBufferImpl.java:270)
! at org.apache.http.impl.conn.DefaultHttpResponseParser.parseHead(DefaultHttpResponseParser.java:140)
! at org.apache.http.impl.conn.DefaultHttpResponseParser.parseHead(DefaultHttpResponseParser.java:57)
! at org.apache.http.impl.io.AbstractMessageParser.parse(AbstractMessageParser.java:260)
! at org.apache.http.impl.DefaultBHttpClientConnection.receiveResponseHeader(DefaultBHttpClientConnection.java:161)
! at sun.reflect.NativeMethodAccessorImpl.invoke0(Native Method)
! at sun.reflect.NativeMethodAccessorImpl.invoke(NativeMethodAccessorImpl.java:57)
! at sun.reflect.DelegatingMethodAccessorImpl.invoke(DelegatingMethodAccessorImpl.java:43)
! at java.lang.reflect.Method.invoke(Method.java:606)
! at org.apache.http.impl.conn.CPoolProxy.invoke(CPoolProxy.java:138)
! at com.sun.proxy.$Proxy25.receiveResponseHeader(Unknown Source)
! at org.apache.http.protocol.HttpRequestExecutor.doReceiveResponse(HttpRequestExecutor.java:271)
! at org.apache.http.protocol.HttpRequestExecutor.execute(HttpRequestExecutor.java:123)
! at org.apache.http.impl.execchain.MainClientExec.execute(MainClientExec.java:253)
! at org.apache.http.impl.execchain.ProtocolExec.execute(ProtocolExec.java:194)
! at org.apache.http.impl.execchain.RetryExec.execute(RetryExec.java:85)
! at org.apache.http.impl.execchain.RedirectExec.execute(RedirectExec.java:108)
! at org.apache.http.impl.client.InternalHttpClient.doExecute(InternalHttpClient.java:186)
! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:82)
! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:106)
! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:57)
! at com.yammer.metrics.scala.Timer.time(Timer.scala:17)
! at com.yammer.metrics.scala.Timer.time(Timer.scala:17)
! at java.util.concurrent.Executors$RunnableAdapter.call(Executors.java:471)
! at java.util.concurrent.FutureTask.runAndReset(FutureTask.java:304)
! at java.util.concurrent.ScheduledThreadPoolExecutor$ScheduledFutureTask.access$301(ScheduledThreadPoolExecutor.java:178)
! at java.util.concurrent.ScheduledThreadPoolExecutor$ScheduledFutureTask.run(ScheduledThreadPoolExecutor.java:293)
! at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1145)
! at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:615)
! at java.lang.Thread.run(Thread.java:744)
]]

local multi_line_test_fields = {
    severity   = 3,
    timestamp  = 1.416501651577e+18,
    class      = "com.domain.substitute.UsersLoop",
    message    = "Error caught in user loop",
    stacktrace = "! java.net.SocketTimeoutException: Read timed out\n! at java.net.SocketInputStream.socketRead0(Native Method)\n! at java.net.SocketInputStream.read(SocketInputStream.java:152)\n! at java.net.SocketInputStream.read(SocketInputStream.java:122)\n! at sun.security.ssl.InputRecord.readFully(InputRecord.java:442)\n! at sun.security.ssl.InputRecord.read(InputRecord.java:480)\n! at sun.security.ssl.SSLSocketImpl.readRecord(SSLSocketImpl.java:927)\n! at sun.security.ssl.SSLSocketImpl.readDataRecord(SSLSocketImpl.java:884)\n! at sun.security.ssl.AppInputStream.read(AppInputStream.java:102)\n! at org.apache.http.impl.io.SessionInputBufferImpl.streamRead(SessionInputBufferImpl.java:136)\n! at org.apache.http.impl.io.SessionInputBufferImpl.fillBuffer(SessionInputBufferImpl.java:152)\n! at org.apache.http.impl.io.SessionInputBufferImpl.readLine(SessionInputBufferImpl.java:270)\n! at org.apache.http.impl.conn.DefaultHttpResponseParser.parseHead(DefaultHttpResponseParser.java:140)\n! at org.apache.http.impl.conn.DefaultHttpResponseParser.parseHead(DefaultHttpResponseParser.java:57)\n! at org.apache.http.impl.io.AbstractMessageParser.parse(AbstractMessageParser.java:260)\n! at org.apache.http.impl.DefaultBHttpClientConnection.receiveResponseHeader(DefaultBHttpClientConnection.java:161)\n! at sun.reflect.NativeMethodAccessorImpl.invoke0(Native Method)\n! at sun.reflect.NativeMethodAccessorImpl.invoke(NativeMethodAccessorImpl.java:57)\n! at sun.reflect.DelegatingMethodAccessorImpl.invoke(DelegatingMethodAccessorImpl.java:43)\n! at java.lang.reflect.Method.invoke(Method.java:606)\n! at org.apache.http.impl.conn.CPoolProxy.invoke(CPoolProxy.java:138)\n! at com.sun.proxy.$Proxy25.receiveResponseHeader(Unknown Source)\n! at org.apache.http.protocol.HttpRequestExecutor.doReceiveResponse(HttpRequestExecutor.java:271)\n! at org.apache.http.protocol.HttpRequestExecutor.execute(HttpRequestExecutor.java:123)\n! at org.apache.http.impl.execchain.MainClientExec.execute(MainClientExec.java:253)\n! at org.apache.http.impl.execchain.ProtocolExec.execute(ProtocolExec.java:194)\n! at org.apache.http.impl.execchain.RetryExec.execute(RetryExec.java:85)\n! at org.apache.http.impl.execchain.RedirectExec.execute(RedirectExec.java:108)\n! at org.apache.http.impl.client.InternalHttpClient.doExecute(InternalHttpClient.java:186)\n! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:82)\n! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:106)\n! at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:57)\n! at com.yammer.metrics.scala.Timer.time(Timer.scala:17)\n! at com.yammer.metrics.scala.Timer.time(Timer.scala:17)\n! at java.util.concurrent.Executors$RunnableAdapter.call(Executors.java:471)\n! at java.util.concurrent.FutureTask.runAndReset(FutureTask.java:304)\n! at java.util.concurrent.ScheduledThreadPoolExecutor$ScheduledFutureTask.access$301(ScheduledThreadPoolExecutor.java:178)\n! at java.util.concurrent.ScheduledThreadPoolExecutor$ScheduledFutureTask.run(ScheduledThreadPoolExecutor.java:293)\n! at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1145)\n! at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:615)\n! at java.lang.Thread.run(Thread.java:744)\n"
}

local function multi_line_logevent()
    local fields = sfl4j.logevent_grammar:match(multi_line_log)
    if not fields then error("match didn't work " .. multi_line_log) end

    assert(fields.severity == multi_line_test_fields.severity, fields.severity)
    assert(fields.timestamp == multi_line_test_fields.timestamp, fields.timestamp)
    assert(fields.class == multi_line_test_fields.class, fields.class)
    assert(fields.message == multi_line_test_fields.message, fields.message)
    assert(fields.Stacktrace == multi_line_test_fields.Stacktrace, fields.Stacktrace)
end

function process()
    single_line_logevent()
    two_line_logevent()
    multi_line_logevent()

    return 0
end
