using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;
using System.Xml.Serialization;
using System.Reflection;

namespace WebDAVServer
{
    public partial class WebDAVServer
    {
        private TcpListener Listener;
        private Thread ConnThread;

        public bool EnableRequestLog = false;
        public bool Started = false;

        internal StringBuilder _RequestMessages = new StringBuilder();
        internal StringBuilder _LogMessages = new StringBuilder();

        public object StatisticsLock = new object();
        public long Connections = 0;
        public long BytesReadHeader = 0;
        public long BytesWrittenHeader = 0;
        public long BytesReadData = 0;
        public long BytesWrittenData = 0;

        public static string Version = "2.9";
        public string DefaultConfigFileName = "WebDAVServer.cfg";
        public string ConfigFilePath = "";
        public string ConfigFileName = "";
        public string ConfigFileStatus = "";

        public event EventHandler LogUpdated = null;

        public object TransferLock = new object();
        public long TransferTotalBytes = 0;
        public long TransferTransferredBytes = 0;

        private LinkedList<Thread> ClientThreads = new LinkedList<Thread>();

        public class SettingsContainer
        {
            public string Version;
            public string Path;
            public int Port;
            public string AuthTokens;
            public int PrefetchCount;
            public int CacheTime;
            public bool ShowJpeg;
            public bool ShowInfos;
            public bool ShowFits;
            public bool ShowDng;
            public bool ShowWav;
        }
        public SettingsContainer Settings = new SettingsContainer();
        public string Statistics
        {
            get
            {
                StringBuilder ret = new StringBuilder();
                
                ret.AppendLine("Statistics:");
                ret.AppendLine(MLVAccessor.Statistics);

                return ret.ToString();
            }
        }

        public WebDAVServer(string[] arg)
        {
            /* default options */
            Settings.Version = Version;
            Settings.Path = GetExecutableRoot();
            Settings.Port = 8082;
            Settings.AuthTokens = "";
            Settings.CacheTime = 5 * 60;
            Settings.PrefetchCount = 10;
            Settings.ShowInfos = true;
            Settings.ShowJpeg = true;
            Settings.ShowFits = true;
            Settings.ShowDng = true;
            Settings.ShowWav = true;

            /* read default config */
            ReadDefaultConfigFile();

            /* try to get alternatives from command line */
            if (arg.Length > 0)
            {
                Settings.Path = (arg[0].Replace("\"", "").Trim() + "\\").Replace("\\\\", "\\");
            }

            if (arg.Length > 1)
            {
                int.TryParse(arg[1], out Settings.Port);
            }

            if (arg.Length > 2)
            {
                Settings.AuthTokens = arg[2];
            }
        }

        public void Start()
        {
            Start(true);
        }

        public void Start(bool force)
        {
            if (Started)
            {
                return;
            }

            if (!force && WebDAVService.Running)
            {
                lock (_LogMessages)
                {
                    _LogMessages.Length = 0;
                    Log("[i]");
                    Log("[i] WebDAVServer NOT started: Windows Service is already running.");
                    Log("[i]");
                    Log("[i] This means that you have installed WebDAVServer as an");
                    Log("[i] daemon into your windows. See setup page to uninstall.");
                    Log("[i]");
                    Log("[i] Please be aware that changes to the config in this mode have");
                    Log("[i] to be written using the \"Write\" button before they take effect.");
                    Log("[i]");
                }
                return;
            }

            try
            {
                /* set up server */
                RequestHandler.RootPath = Settings.Path;
                RequestHandler.AuthTokens = Settings.AuthTokens;
                Listener = new TcpListener(IPAddress.Any, Settings.Port);
                Listener.Start(10);

                ConnThread = new Thread(() =>
                {
                    try
                    {
                        lock (_LogMessages)
                        {
                            _LogMessages.Length = 0;
                            Log("[i] WebDAVServer started.");
                            if (Listener.LocalEndpoint != null)
                            {
                                Log("[i] Listening on " + Listener.LocalEndpoint.ToString());
                            }
                            else
                            {
                                Log("[i] ERROR - Failed fo bind port");
                            }
                            Log("[i] Root path '" + RequestHandler.RootPath + "'");
                        }

                        while (true)
                        {
                            Socket sock = Listener.AcceptSocket();
                            Connections++;

                            Thread clientThread = new Thread(() =>
                            {
                                try
                                {
                                    HandleConnection(sock);
                                }
                                catch (Exception e)
                                {
                                }
                                CleanupClientThreads();
                            });

                            try
                            {
                                clientThread.Priority = ThreadPriority.Highest;
                            }
                            catch (Exception e)
                            {
                            }
                            clientThread.Name = "Handle Connection";
                            clientThread.Start();

                            lock (ClientThreads)
                            {
                                ClientThreads.AddLast(clientThread);
                            }
                        }
                    }
                    catch (SocketException e)
                    {
                        Log("[E] WebDAVServer failed to start. (" + e.GetType() + ")");
                    }
                });
                ConnThread.Start();

                Started = true;
            }
            catch (SocketException e)
            {
                switch (e.SocketErrorCode)
                {
                    case SocketError.AccessDenied:
                        Log("[E] WebDAVServer failed to start: You are not allowed to use this port (start as administrator)");
                        break;
                    case SocketError.AddressAlreadyInUse:
                        Log("[E] WebDAVServer failed to start: This port is already in use");
                        break;
                    default:
                        Log("[E] WebDAVServer failed to start: " + e.SocketErrorCode + " (" + e.Message + ")");
                        break;
                }
            }
            catch (Exception e)
            {
                Log("[E] WebDAVServer failed to start: " + e.GetType() + " (" + e.Message + ")");
            }
        }

        public void Stop()
        {
            Stop(null);
        }

        public void Stop(string reason)
        {
            if (reason != null)
            {
                Log("[i] Stopping Server. Reason: " + reason);
            }
            else
            {
                Log("[i] Stopping Server.");
            }

            Started = false;

            if (Listener != null)
            {
                Listener.Stop();
                Listener = null;
            }

            if (ConnThread != null)
            {
                ConnThread.Abort();
                ConnThread = null;
            }

            foreach (Thread thread in ClientThreads)
            {
                thread.Abort();
            }

            CleanupClientThreads();
        }

        public void Restart()
        {
            Stop();
            Start(false);
        }

        public class TransferInfo
        {
            private long TransferSize = 0;
            private long Transferred = 0;
            WebDAVServer Server;

            public TransferInfo(WebDAVServer server, long size)
            {
                Server = server;

                lock (Server.TransferLock)
                {
                    TransferSize = size;
                    Server.TransferTotalBytes += size;
                }

                Update();
            }

            public void BlockTransferred(long size)
            {
                lock (Server.TransferLock)
                {
                    Transferred += size;
                    Server.TransferTransferredBytes += size;

                    if (Server.TransferTransferredBytes == Server.TransferTotalBytes)
                    {
                        Server.TransferTransferredBytes = 0;
                        Server.TransferTotalBytes = 0;
                    }
                }

                Update();
            }

            public void TransferFinished()
            {
                lock (Server.TransferLock)
                {
                    Server.TransferTotalBytes -= TransferSize;
                    Server.TransferTransferredBytes -= Transferred;
                    TransferSize = 0;
                    Transferred = 0;
                }

                Update();
            }

            public void Update()
            {
                lock (Server.TransferLock)
                {
                    if (Server.TransferTransferredBytes == Server.TransferTotalBytes)
                    {
                        Server.TransferTransferredBytes = 0;
                        Server.TransferTotalBytes = 0;
                    }
                }
            }
        }

        public void TransferGetStats(out long total, out long transferred)
        {
            lock (TransferLock)
            {
                total = TransferTotalBytes;
                transferred = TransferTransferredBytes;
            }

        }
        private void ReadDefaultConfigFile()
        {
            ConfigFilePath = GetExecutableRoot();
            ConfigFileName = ConfigFilePath + "\\" + DefaultConfigFileName;

            if (File.Exists(ConfigFileName))
            {
                ConfigFileStatus = "existing";
                ReadConfigFile(ConfigFileName);
            }
            else
            {
                ConfigFileStatus = "missing";
            }
        }

        public string GetExecutableRoot()
        {
            string executable = Assembly.GetAssembly(typeof(WebDAVServer)).Location;
            return Path.GetDirectoryName(executable);
        }

        public void ReadConfigFile(string fileName)
        {
            SettingsContainer settings = null;
            try
            {
                XmlSerializer ser = new XmlSerializer(typeof(SettingsContainer));
                FileStream stream = new FileStream(fileName, FileMode.Open);
                try
                {
                    settings = (SettingsContainer)ser.Deserialize(stream);
                    if (Version == settings.Version)
                    {
                        Settings = settings;
                        ConfigFileStatus += ", successfully read";
                    }
                    else
                    {
                        ConfigFileStatus += ", successfully read but version mismatch";
                    }
                }
                catch (Exception ex)
                {
                    ConfigFileStatus += ", caused " + ex.GetType().Name;
                }
                finally
                {
                    stream.Close();
                }
            }
            catch (Exception ex)
            {
                ConfigFileStatus += ", caused " + ex.GetType().Name;
            }

        }

        public void SaveConfigFile(string fileName)
        {
            try
            {
                XmlSerializer ser = new XmlSerializer(typeof(SettingsContainer));
                StreamWriter writer = new StreamWriter(fileName, false);
                try
                {
                    ser.Serialize(writer, Settings);
                }
                finally
                {
                    writer.Close();
                }
            }
            catch (Exception ex)
            {
            }
        }

        public string RequestMessages
        {
            get
            {
                lock (_RequestMessages)
                {
                    return _RequestMessages.ToString();
                }
            }
            set
            {
                _RequestMessages = new StringBuilder(value);
            }
        }

        public string LogMessages
        {
            get
            {
                lock (_LogMessages)
                {
                    return _LogMessages.ToString();
                }
            }
            set
            {
                _LogMessages = new StringBuilder(value);
            }
        }

        private void CleanupClientThreads()
        {
            LinkedList<Thread> remove = new LinkedList<Thread>();

            lock (ClientThreads)
            {
                foreach (Thread thread in ClientThreads)
                {
                    if (!thread.IsAlive)
                    {
                        remove.AddLast(thread);
                    }
                }

                foreach (Thread thread in remove)
                {
                    ClientThreads.Remove(thread);
                }
            }
        }

        public void Log(string msg)
        {
            lock (_LogMessages)
            {
                int length = _LogMessages.Length;
                if (length > 2 * 1024 * 1024)
                {
                    _LogMessages.Length = 0;
                    _LogMessages.AppendLine("(Truncated " + length + " characters to prevent slowdown)");
                }
                _LogMessages.AppendLine(msg);

                if (LogUpdated != null)
                {
                    LogUpdated(this, null);
                }
            }
        }

        public void LogRequest(string msg)
        {
            if (!EnableRequestLog)
            {
                return;
            }

            lock (_RequestMessages)
            {
                int length = _RequestMessages.Length;
                if (length > 2 * 1024 * 1024)
                {
                    _RequestMessages.Length = 0;
                    _RequestMessages.AppendLine("(Truncated " + length + " characters to prevent slowdown)");
                }
                _RequestMessages.AppendLine(msg);
            }
        }

        private void HandleConnection(Socket sock)
        {
            NetworkStream stream = new NetworkStream(sock);
            string line = null;
            bool error = false;
            bool keepAlive = false;
            DateTime startTime = DateTime.Now;

            sock.ReceiveTimeout = RequestHandler.Timeout * 100;
            sock.Blocking = false;
            sock.NoDelay = true;
            sock.SendBufferSize = 16 * 1024;
            sock.UseOnlyOverlappedIO = true;

            string type = "";
            string path = "";

            do
            {
                bool first = true;
                RequestHandler handler = null;

                do
                {
                    line = null;
                    try
                    {
                        line = ReadLine(stream);
                        BytesReadHeader += line.Length + 2;
                    }
                    catch (ThreadAbortException e)
                    {
                        keepAlive = false;
                        error = true;
                        break;
                    }
                    catch (IOException e)
                    {
                        keepAlive = false;
                        error = true;
                        break;
                    }
                    catch (Exception e)
                    {
                        keepAlive = false;
                        error = true;
                        break;
                    }

                    /* connection timed out or closed */
                    if (line == null)
                    {
                        sock.Close();
                        LogRequest("  (Socket closed)");
                        return;
                    }

                    if (first)
                    {
                        LogRequest("  (Connection from " + sock.RemoteEndPoint + ")");
                    }
                    LogRequest("< " + line);

                    /* not an empty line? */
                    if (line != "")
                    {
                        /* the first line contains the request */
                        if (first)
                        {
                            if (line.Contains(' '))
                            {
                                type = line.Substring(0, line.IndexOf(' '));
                                path = line.Substring(line.IndexOf(' ')).Trim();
                                try
                                {
                                    switch (type)
                                    {
                                        case "OPTIONS":
                                            handler = new OptionsHandler(this, path);
                                            break;

                                        case "PROPFIND":
                                            handler = new PropFindHandler(this, path);
                                            break;

                                        case "GET":
                                            handler = new GetHandler(this, path);
                                            break;

                                        case "HEAD":
                                            handler = new HeadHandler(this, path);
                                            break;

                                        case "PUT":
                                            handler = new PutHandler(this, path);
                                            break;

                                        case "LOCK":
                                            handler = new LockHandler(this, path);
                                            break;

                                        case "UNLOCK":
                                            handler = new UnlockHandler(this, path);
                                            break;

                                        case "DELETE":
                                            handler = new DeleteHandler(this, path);
                                            break;

                                        case "MOVE":
                                            handler = new MoveHandler(this, path);
                                            break;

                                        case "COPY":
                                            handler = new CopyHandler(this, path);
                                            break;

                                        case "MKCOL":
                                            handler = new MkColHandler(this, path);
                                            break;

                                        case "PROPPATCH":
                                            handler = new PropPatchHandler(this, path);
                                            break;
                                            
                                        default:
                                            handler = new RequestHandler(this, "/");
                                            break;
                                    }
                                }
                                catch (IOException e)
                                {
                                    Log("[i] Connection from " + sock.RemoteEndPoint + " (" + type + ") had IOException");
                                }
                                catch (Exception e)
                                {
                                    Log("[E] '" + e.GetType().ToString() + "' in connection from " + sock.RemoteEndPoint + " (" + type + ")");
                                }
                            }

                            first = false;
                        }
                        else
                        {
                            try
                            {
                                handler.AddHeaderLine(line);
                            }
                            catch (IOException e)
                            {
                                /* just close */
                                sock.Close();
                                LogRequest("  (Socket closed)");
                                return;
                            }
                            catch (Exception e)
                            {
                                Log("[E] '" + e.GetType().ToString() + "' in connection from " + sock.RemoteEndPoint + " (AddHeaderLine)");
                            }
                            //stream.Flush();
                        }
                    }
                } while (line != "");

                if(handler == null)
                {
                    Log("[E] Empty request in connection from " + sock.RemoteEndPoint + " (HandleContent/HandleRequest)");
                    handler.KeepAlive = false;
                    return;
                }

                if (!error)
                {
                    try
                    {
                        if (handler.RequestContentLength > 0)
                        {
                            handler.HandleContent(stream);
                        }

                        handler.HandleRequest(stream);
                        stream.Flush();
                    }
                    catch (FileNotFoundException e)
                    {
                        Log("[E] 404 '" + e.GetType().ToString() + ": " + e.Message + "' in connection from " + sock.RemoteEndPoint + " (HandleContent/HandleRequest)");

                        /* respond with error */
                        handler = new RequestHandler(this, "/");
                        handler.StatusCode = RequestHandler.GetStatusCode(404);
                        handler.KeepAlive = false;
                        handler.HandleRequest(stream);
                        stream.Flush();
                    }
                    catch (SocketException e)
                    {
                        Log("[E] '" + e.GetType().ToString() + ": " + e.Message + "' in connection from " + sock.RemoteEndPoint + " (HandleContent/HandleRequest)");
                        handler.KeepAlive = false;
                        return;
                    }
                    catch (UnauthorizedAccessException e)
                    {
                        Log("[i] 403 '" + e.GetType().ToString() + ": " + e.Message + "' in connection from " + sock.RemoteEndPoint + " (HandleContent/HandleRequest)");

                        /* respond with error */
                        handler = new RequestHandler(this, "/");
                        handler.StatusCode = RequestHandler.GetStatusCode(403);
                        handler.KeepAlive = false;
                        handler.HandleRequest(stream);
                        stream.Flush();
                    }
                    catch (Exception e)
                    {
                        Log("[E] 500 '" + e.GetType().ToString() + ": " + e.Message + "' in connection from " + sock.RemoteEndPoint + " (HandleContent/HandleRequest)");

                        /* respond with error */
                        handler = new RequestHandler(this, "/");
                        handler.StatusCode = RequestHandler.GetStatusCode(500);
                        handler.KeepAlive = false;
                        handler.HandleRequest(stream);
                        stream.Flush();
                    }


                    if (EnableRequestLog)
                    {
                        DateTime endTime = DateTime.Now;
                        Log("[i] Connection from " + sock.RemoteEndPoint + " (" + type + " " + path + ") took " + (endTime - startTime).TotalMilliseconds + " ms (" + handler.StatusCode + ")");
                    }
                    LogRequest("");

                    lock (StatisticsLock)
                    {
                        BytesWrittenHeader += handler.BytesWrittenHeader;
                        BytesWrittenData += handler.BytesWrittenData;
                        BytesReadHeader += handler.BytesReadHeader;
                        BytesReadData += handler.BytesReadData;
                    }

                    keepAlive = handler.KeepAlive;

                    /* windows isnt really using keepalive :( */
                    keepAlive = false;
                }
            } while (keepAlive);

            sock.Close();
            LogRequest("  (Socket closed)");
        }

        private string ReadLine(NetworkStream stream)
        {
            StringBuilder line = new StringBuilder();

            try
            {
                while (true)
                {
                    int ch = stream.ReadByte();

                    if (ch < 0 && line.Length == 0)
                    {
                        throw new EndOfStreamException();
                    }
                    else if (ch < 0)
                    {
                        break;
                    }
                    else
                    {
                        if ((char)ch != '\n' && (char)ch != '\r')
                        {
                            line.Append((char)ch);
                        }
                        else if ((char)ch != '\r')
                        {
                            break;
                        }
                    }
                }
            }
            catch (Exception e)
            {
            }

            return line.ToString();
        }
    }
}
