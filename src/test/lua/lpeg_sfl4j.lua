local sfl4j = require("sfl4j")

local function single_line_logevent()
    local single_line_log = 'ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name\n'
    local single_line_test_fields = {
        Level        = "ERROR",
        Timestamp    = "2014-11-21 16:35:59,501",
        Class   = "com.domain.client.jobs.OutgoingQueue",
        Message = "Error handling output file with job job-name"
    }

    local fields = sfl4j.logevent_grammar:match(single_line_log)
    if not fields then error("match didn't work " .. single_line_log) end
    assert(fields.Level == single_line_test_fields.Level, fields.Level)
    assert(fields.Timestamp == single_line_test_fields.Timestamp, fields.Timestamp)
    assert(fields.Class == single_line_test_fields.Class, fields.Class)
    assert(fields.Message == single_line_test_fields.Message, fields.Message)
end

local function two_line_logevent()
    local two_line_log = [[
ERROR [2014-11-20 16:40:51,577] com.domain.substitute.UsersLoop: Error caught in user loop
! java.net.SocketTimeoutException: Read timed out
    ]]
    local two_line_test_fields = {
        Level        = "ERROR",
        Timestamp    = "2014-11-20 16:40:51,577",
        Class   = "com.domain.substitute.UsersLoop",
        Message = "Error caught in user loop",
        Stacktrace = "! java.net.SocketTimeoutException: Read timed out\n"
    }

    local fields = sfl4j.logevent_grammar:match(two_line_log)
    if not fields then error("match didn't work " .. two_line_log) end
    assert(fields.Level == two_line_test_fields.Level, fields.Level)
    assert(fields.Timestamp == two_line_test_fields.Timestamp, fields.Timestamp)
    assert(fields.Class == two_line_test_fields.Class, fields.Class)
    assert(fields.Message == two_line_test_fields.Message, fields.Message)
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
    Level     = "ERROR",
    Timestamp = "2014-11-20 16:40:51,577",
    Class     = "com.domain.substitute.UsersLoop",
    Message   = "Error caught in user loop",
    Stacktrace = [[
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
}

local function multi_line_logevent()
    local fields = sfl4j.logevent_grammar:match(multi_line_log)
    if not fields then error("match didn't work " .. multi_line_log) end
    assert(fields.Level == multi_line_test_fields.Level, fields.Level)
    assert(fields.Timestamp == multi_line_test_fields.Timestamp, fields.Timestamp)
    assert(fields.Class == multi_line_test_fields.Class, fields.Class)
    assert(fields.Message == multi_line_test_fields.Message, fields.Message)
end

function process()
    single_line_logevent()
    two_line_logevent()
    multi_line_logevent()

    return 0
end
