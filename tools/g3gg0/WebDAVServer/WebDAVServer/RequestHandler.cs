using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml;
using System.Globalization;
using System.Web;
using System.Net.Sockets;
using System.Collections;
using Microsoft.Win32;
using System.Windows.Forms;

namespace WebDAVServer
{
    public class RequestHandler
    {
        public static string RootPath = "c:\\temp\\";
        public static string ImplementedHandlers = "OPTIONS,PROPFIND,GET,DELETE,COPY,MOVE,LOCK";
        public static string AuthTokens = "";
        public static string AuthorizationRealm = "WebDAVServer by g3gg0.de";

        protected WebDAVServer Server = null;
        public static int Timeout = 1;

        public long BytesReadHeader = 0;
        public long BytesWrittenHeader = 0;
        public long BytesReadData = 0;
        public long BytesWrittenData = 0;

        public bool KeepAlive = false;
        public string DestinationHeaderLine = "";
        public string HostHeaderLine = "";
        public string AuthorizationHeaderLine = "";
        public string DepthHeaderLine = "";

        public byte[] RequestContent = new byte[0];
        public long RequestContentLength = 0;

        public string StatusCode = GetStatusCode(500);
        public string MimeType = "text/plain";
        public string ResponseTextContent = "";

        public long ResponseBinaryContentLength = 0;
        public byte[] ResponseBinaryContent = null;
        public ArrayList HeaderLines = new ArrayList();

        protected string RequestPath = "";
        protected string BaseHref = "";
        protected bool SkipRequestLogging = false;
        public bool SendHeaderOnly = false;

        public enum FileType
        {
            PureVirtualDir,
            PureVirtualFile,
            Redirected,
            Direct,
            Invalid
        }

        public class VirtualFileType
        {
            public FileType Type;
            public string RequestPath;
            public string LocalPath;

            public string MlvFilePath;
            public string MlvFileContent;
            public string MlvStoreDir;
            public string MlvBaseName;

            public static VirtualFileType Resolve(string request)
            {
                VirtualFileType type = new VirtualFileType();

                type.RequestPath = request;

                bool containsMlv = request.ToUpper().Contains(".MLV");
                bool containsRaw = request.ToUpper().Contains(".RAW");

                string ext = "";
                string storeDirExt = "";

                if (containsMlv)
                {
                    ext = ".MLV";
                    storeDirExt = ".MLD";
                }
                else
                {
                    ext = ".RAW";
                    storeDirExt = ".RAD";
                }

                type.LocalPath = GetLocalName(RootPath, request);
                type.MlvFilePath = GetMlvFileName(type.LocalPath);

                if(!File.Exists(type.MlvFilePath))
                {
                    type.Type = FileType.Direct;
                    type.LocalPath = GetLocalName(RootPath, request);
                    return type;
                }

                FileInfo mlvFileInfo = new FileInfo(type.MlvFilePath);
                type.MlvBaseName = mlvFileInfo.Name.ToUpper().Replace(ext, "");

                if (mlvFileInfo.Directory != null)
                {
                    type.MlvStoreDir = mlvFileInfo.Directory.FullName + Path.DirectorySeparatorChar + type.MlvBaseName + storeDirExt;
                }

                if (request.ToUpper().EndsWith(ext))
                {
                    type.LocalPath = "";
                    type.Type = FileType.PureVirtualDir;
                    return type;
                }
                else if (containsMlv || containsRaw)
                {
                    /* strip the requested file within that directory. e.g. 'M12-3456_<anything>' or 'foo.txt'  */
                    string requestedFile = type.LocalPath.Replace(type.MlvFilePath + Path.DirectorySeparatorChar, "");

                    /* is this not a M12-3456_<anything> filename? or is it a directory? */
                    if (!requestedFile.StartsWith(type.MlvBaseName) || requestedFile.Contains(Path.DirectorySeparatorChar))
                    {
                        type.Type = FileType.Redirected;
                        type.LocalPath = type.MlvStoreDir + Path.DirectorySeparatorChar + requestedFile;

                        return type;
                    }

                    string content = requestedFile.Replace(type.MlvBaseName, "").Trim('_');

                    /* save location in virtual directory in the "LocalPath" field */
                    type.LocalPath = type.MlvStoreDir + Path.DirectorySeparatorChar + requestedFile;

                    if (File.Exists(type.LocalPath))
                    {
                        type.Type = FileType.Redirected;
                        return type;
                    }

                    if (!MLVAccessor.FileExists(type.MlvFilePath, content))
                    {
                        type.Type = FileType.Redirected;
                        return type;
                    }

                    type.Type = FileType.PureVirtualFile;
                    type.MlvFileContent = content.ToUpper();
                    return type;
                }

                type.Type = FileType.Direct;
                type.LocalPath = GetLocalName(RootPath, request);
                return type;
            }
        }

        public void AddDavHeader()
        {
            HeaderLines.Add("DAV: 1,2");
            HeaderLines.Add("DAV: version-control,checkout,working-resource");
            HeaderLines.Add("DAV: merge,baseline,activity,version-controlled-collection");
            HeaderLines.Add("DAV: http://subversion.tigris.org/xmlns/dav/svn/depth");
            HeaderLines.Add("DAV: http://subversion.tigris.org/xmlns/dav/svn/log-revprops");
            HeaderLines.Add("DAV: http://subversion.tigris.org/xmlns/dav/svn/partial-replay");
            HeaderLines.Add("DAV: http://subversion.tigris.org/xmlns/dav/svn/mergeinfo");
            HeaderLines.Add("DAV: <http://apache.org/dav/propset/fs/1>");
            HeaderLines.Add("MS-Author-Via: DAV");
            HeaderLines.Add("Allow: " + ImplementedHandlers);
        }

        public static string GetMlvFileName(string localPath)
        {
            string[] splits = localPath.Split(Path.DirectorySeparatorChar);
            string path = "";

            foreach (string elem in splits)
            {
                path += elem;
                if (elem.ToUpper().EndsWith(".MLV") || elem.ToUpper().EndsWith(".RAW"))
                {
                    if (File.Exists(path))
                    {
                        return path;
                    }
                }
                path += Path.DirectorySeparatorChar;
            }

            return path;
        }

        public RequestHandler(WebDAVServer server, string request)
        {
            Server = server;
            if (request.Contains(' '))
            {
                request = request.Substring(0, request.IndexOf(' '));
            }

            RequestPath = UnescapeSpecialChars(request);
        }

        public static string GetStatusCode(int number)
        {
            string response = "HTTP/1.1 ";

            switch (number)
            {
                case 200:
                    response += number + " OK";
                    break;
                case 201:
                    response += number + " Created";
                    break;
                case 204:
                    response += number + " No Content";
                    break;
                case 207:
                    response += number + " OK";
                    break;
                case 302:
                    response += number + " Found";
                    break;
                case 401:
                    response += number + " Unauthorized";
                    break;
                case 403:
                    response += number + " Forbidden";
                    break;
                case 404:
                    response += number + " Not found";
                    break;
                case 409:
                    response += number + " Conflict";
                    break;
                case 500:
                    response += number + " Internal Server Error";
                    break;
                case 507:
                    response += number + " Insufficient Storage";
                    break;
                default:
                    response += number + " Unknown";
                    break;
            }

            return response;
        }

        public void AddHeader(ref string listing)
        {
            listing += "<pre><a href=\"/\">[Index]</a> | <a href=\"/log\">[Log]</a> | <a href=\"/debug\">[Info]</a></pre>";
        }

        public void AddFooter(ref string listing)
        {
            listing += "<hr><address>WebDAVServer v" + WebDAVServer.Version + " by g3gg0.de</address>";
        }

        public virtual void AddHeaderLine(string line)
        {
            try
            {
                if (line.Contains(':'))
                {
                    string type = line.Substring(0, line.IndexOf(' '));
                    string data = line.Substring(line.IndexOf(' ') + 1);

                    switch (type)
                    {
                        case "User-Agent:":
                            break;

                        case "Authorization:":
                            AuthorizationHeaderLine = data;
                            break;

                        case "Destination:":
                            DestinationHeaderLine = UnescapeSpecialChars(data);
                            break;

                        case "Host:":
                            HostHeaderLine = data;
                            break;

                        case "Depth:":
                            DepthHeaderLine = data;
                            break;

                        case "Connection:":
                            if (data.Contains("Keep-Alive"))
                            {
                                KeepAlive = true;
                            }
                            break;

                        case "Content-Length:":
                            long.TryParse(data, out RequestContentLength);
                            break;
                    }
                }
            }
            catch (Exception e)
            {
            }

            BaseHref = "http://" + HostHeaderLine;
        }

        public virtual void HandleContent(Stream stream)
        {
            /* read all into a buffer and convert to string */
            byte[] buffer = new byte[RequestContentLength];
            stream.Read(buffer, 0, (int)RequestContentLength);

            BytesReadData += RequestContentLength;

            RequestContent = buffer;

            /* dump content */
            ASCIIEncoding enc = new ASCIIEncoding();
            string text = enc.GetString(RequestContent);

            foreach (string line in text.Split('\n'))
            {
                Server.LogRequest("< " + line.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;").Replace("\"", "&quot;"));
            }
        }

        public virtual void HandleRequest(Stream stream)
        {
            StringBuilder response = new StringBuilder();

            /* authentication required? */
            if (IsAccessDenied())
            {
                StatusCode = GetStatusCode(401);
                HeaderLines.Clear();
                HeaderLines.Add("WWW-Authenticate: Basic realm=" + AuthorizationRealm);
                ResponseTextContent = "";
                ResponseBinaryContentLength = 0;

                if (ResponseBinaryContent != null)
                {
                    ResponseBinaryContent = null;
                }
            }

            response.AppendLine(StatusCode);
            response.AppendLine("Date: " + DateTime.Now.ToUniversalTime().ToString("r", CultureInfo.InvariantCulture));
            response.AppendLine("Server: WebDAVServer/" + WebDAVServer.Version + " (g3gg0.de)");

            foreach (string line in HeaderLines)
            {
                response.AppendLine(line);
            }
            HeaderLines.Clear();

            response.AppendLine("Content-Length: " + (ResponseTextContent.Length + ResponseBinaryContentLength));

            if (KeepAlive)
            {
                response.AppendLine("Keep-Alive: timeout=" + Timeout + ", max=" + (Timeout + 10));
                response.AppendLine("Connection: Keep-Alive");
            }
            else
            {
                response.AppendLine("Connection: Close");
            }

            response.AppendLine("Content-Type: " + MimeType);
            response.AppendLine("");

            byte[] textHeaderData = Encoding.ASCII.GetBytes(response.ToString());
            stream.Write(textHeaderData, 0, textHeaderData.Length);
            BytesWrittenHeader += textHeaderData.Length;

            foreach (string line in response.ToString().Split('\n'))
            {
                Server.LogRequest("> " + line.Replace("\r", ""));
            }

            if (!SendHeaderOnly)
            {
                if (ResponseTextContent.Length > 0)
                {
                    WebDAVServer.TransferInfo info = new WebDAVServer.TransferInfo(Server, ResponseTextContent.Length);
                    int written = 0;

                    byte[] respTextData = Encoding.ASCII.GetBytes(ResponseTextContent);
                    try
                    {
                        stream.Write(respTextData, written, respTextData.Length);

                        written += respTextData.Length;
                        BytesWrittenHeader += respTextData.Length;
                        info.BlockTransferred(respTextData.Length);
                    }
                    catch (Exception e)
                    {
                    }

                    info.TransferFinished();

                    if (!SkipRequestLogging)
                    {
                        Server.LogRequest(ResponseTextContent.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;").Replace("\"", "&quot;"));
                    }
                    SkipRequestLogging = false;

                    ResponseTextContent = "";
                }

                if (ResponseBinaryContentLength > 0)
                {
                    WebDAVServer.TransferInfo info = new WebDAVServer.TransferInfo(Server, ResponseBinaryContentLength);
                    long written = 0;

                    try
                    {
                        do
                        {
                            stream.Write(ResponseBinaryContent, 0, ResponseBinaryContent.Length);
                            written += ResponseBinaryContent.Length;
                            BytesWrittenData += ResponseBinaryContent.Length;
                            info.BlockTransferred(ResponseBinaryContent.Length);
                        } while (true);
                    }
                    catch (Exception e)
                    {
                    }

                    info.TransferFinished();

                    Server.LogRequest("> (binary content of size " + written + ")");

                    stream.Flush();
                }

                if (ResponseBinaryContent != null)
                {
                    ResponseBinaryContent = null;
                }
            }
        }

        public bool IsAccessDenied()
        {
            bool accessDenied = true;

            if (AuthTokens != "")
            {
                if (AuthorizationHeaderLine != null)
                {
                    string[] data = AuthorizationHeaderLine.Split(' ');

                    if (data.Length > 0)
                    {
                        string realm = data[0];
                        string codedAuth = data[data.Length - 1];

                        byte[] authBytes = Convert.FromBase64String(codedAuth);
                        UTF8Encoding enc = new UTF8Encoding();
                        string authString = enc.GetString(authBytes);

                        string[] tokens = AuthTokens.Split(',');

                        foreach (string token in tokens)
                        {
                            if (authString == token)
                            {
                                accessDenied = false;
                            }
                        }
                    }
                }
            }
            else
            {
                accessDenied = false;
            }

            return accessDenied;
        }

        public static string StreamToString(Stream stream)
        {
            string ret = "";

            using (StreamReader reader = new StreamReader(stream, Encoding.UTF8))
            {
                reader.BaseStream.Position = 0;
                ret = reader.ReadToEnd();
            }

            return ret;
        }

        public string GetDestinationLocation()
        {
            /* check if its the same server address */
            if (DestinationHeaderLine.StartsWith("http://" + HostHeaderLine))
            {
                string path = DestinationHeaderLine.Replace("http://" + HostHeaderLine, "/");

                path = path.Replace("//", "/");
                    path = UnescapeSpecialChars(path);

                return path;
            }

            return null;
        }

        /* collapse relative paths like test/test/../../../ or c:\test\..\test */
        public static string CollapsePath(string path, char separator, bool withPostfix)
        {
            Stack<string> folders = new Stack<string>();

            foreach (string folder in path.Split(separator))
            {
                if (folder == "..")
                {
                    if (folders.Count > 0)
                    {
                        folders.Pop();
                    }
                }
                else if (folder == "")
                {
                    folders.Push(".");
                }
                else
                {
                    folders.Push(folder);
                }
            }

            string ret = "";
            while (folders.Count > 0)
            {
                string name = folders.Pop();
                if (name != ".")
                {
                    if (withPostfix || ret != "")
                    {
                        ret = name + separator + ret;
                    }
                    else
                    {
                        ret = name + ret;
                    }
                }
            }

            return ret;
        }

        /* remove double-slashes, encoding etc */
        public static string CleanPath(string request)
        {
            char separator = '/';

            //request = Uri.UnescapeDataString(request);

            string newRequest = request;
            do
            {
                request = newRequest;
                newRequest = request.Replace("" + separator + separator, "" + separator);
            } while (request != newRequest);

            request = CollapsePath(newRequest, '/', true);

            return request;
        }

        private string UnescapeSpecialChars(string decoded)
        {
            ArrayList chars = new ArrayList();
            byte[] data = Encoding.UTF8.GetBytes(decoded);

            for (int pos = 0; pos < data.Length; pos++)
            {
                if (pos + 3 <= data.Length && data[pos] == '%' )
                {
                    string val = (char)data[pos + 1] + "" + (char)data[pos + 2];
                    byte ch = (byte)int.Parse(val, NumberStyles.HexNumber);

                    chars.Add(ch);

                    /* skip next 2 characters */
                    pos += 2;
                }
                else
                {
                    chars.Add(data[pos]);
                }
            }

            return Encoding.UTF8.GetString((byte[])chars.ToArray(typeof(byte)));
        }

        public string EscapeSpecialChars(string realName)
        {
            StringBuilder builder = new StringBuilder();

            foreach (byte ch in Encoding.UTF8.GetBytes(realName))
            {
                bool escape = true;

                if (ch >= '0' && ch <= '9')
                {
                    escape = false;
                }

                if (ch >= 'a' && ch <= 'z')
                {
                    escape = false;
                }

                if (ch >= 'A' && ch <= 'Z')
                {
                    escape = false;
                }

                if (ch == '.' || ch == '-' || ch == '_'/* || ch == ' '*/ || ch == '/')
                {
                    escape = false;
                }

                if (escape)
                {
                    builder.Append("%" + String.Format("{0:x2}", (int)ch));
                }
                else
                {
                    builder.Append((char)ch);
                }
            }

            return builder.ToString();
        }

        /* convert a URL path into a local filename */
        public static string GetLocalName(string root, string path)
        {
            if (root == null || path == null)
            {
                return null;
            }

            string decoded = CleanPath(path);

            decoded = decoded.Replace('/', '\\');
            decoded = root + "\\" + decoded;
            decoded = CollapsePath(decoded, '\\', false);

            if (Directory.Exists(decoded))
            {
                decoded = decoded + "\\";
            }

            return decoded;
        }

        public static string GetMimeType(string file)
        {
            string mimeType = "application/unknown";
            string ext = System.IO.Path.GetExtension(file).ToLower();
            RegistryKey regKey = Registry.ClassesRoot.OpenSubKey(ext);
            if (regKey != null && regKey.GetValue("Content Type") != null)
                mimeType = regKey.GetValue("Content Type").ToString();
            return mimeType;
        }
    }


    public class PropFindHandler : RequestHandler
    {
        public PropFindHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            using (Stream mem = new MemoryStream())
            {
                VirtualFileType type = VirtualFileType.Resolve(RequestPath);

                switch (type.Type)
                {
                    case FileType.Invalid:
                        StatusCode = GetStatusCode(400);
                        break;

                    case FileType.PureVirtualDir:
                        {
                            try
                            {
                                XmlTextWriter xml = new XmlTextWriter(mem, Encoding.UTF8);
                                FileInfo mlvFileInfo = new FileInfo(type.MlvFilePath);

                                xml.Formatting = Formatting.Indented;
                                xml.IndentChar = ' ';
                                xml.Indentation = 1;
                                xml.WriteStartDocument();

                                xml.WriteStartElement("D", "multistatus", "DAV:");

                                /* infos about current directory */
                                AddDirectoryInfo(xml, mlvFileInfo, RequestPath);

                                int depth = 0;

                                if (DepthHeaderLine == "1")
                                {
                                    depth = 1;
                                }
                                else if (DepthHeaderLine.ToLower() == "infinity")
                                {
                                    depth = int.MaxValue;
                                }

                                /* add children only if propfind is called with a depth > 0 */
                                if (depth > 0)
                                {
                                    if (Program.Instance.Server.Settings.ShowInfos)
                                    {
                                        foreach (string file in MLVAccessor.GetInfoFields(mlvFileInfo.FullName))
                                        {
                                            string virtFile = ValidateFileName(type.MlvBaseName + "__" + file);
                                            DAVFileDirInfo mlvMiscInfo = new DAVFileDirInfo(virtFile);

                                            mlvMiscInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".txt");
                                            mlvMiscInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".txt");
                                            mlvMiscInfo.Length = 0;
                                            mlvMiscInfo.FullName = RequestPath + "/" + mlvMiscInfo.Name;

                                            AddFileInfo(xml, mlvMiscInfo, mlvMiscInfo.FullName);
                                        }

                                        DAVFileDirInfo mlvInfoInfo = new DAVFileDirInfo(type.MlvBaseName + ".txt");

                                        mlvInfoInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".txt");
                                        mlvInfoInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".txt");
                                        mlvInfoInfo.Length = MLVAccessor.GetFileSize(mlvFileInfo.FullName, ".txt");
                                        mlvInfoInfo.FullName = RequestPath + "/" + mlvInfoInfo.Name;

                                        if (!File.Exists(type.MlvStoreDir + Path.DirectorySeparatorChar + mlvInfoInfo.Name))
                                        {
                                            AddFileInfo(xml, mlvInfoInfo, mlvInfoInfo.FullName);
                                        }
                                    }

                                    /* wav file*/
                                    if (Program.Instance.Server.Settings.ShowWav)
                                    {
                                        if (MLVAccessor.HasAudio(mlvFileInfo.FullName))
                                        {
                                            DAVFileDirInfo mlvWavInfo = new DAVFileDirInfo(type.MlvBaseName + ".wav");

                                            mlvWavInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".wav");
                                            mlvWavInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, ".wav");
                                            mlvWavInfo.Length = MLVAccessor.GetFileSize(mlvFileInfo.FullName, ".wav");
                                            mlvWavInfo.FullName = RequestPath + "/" + mlvWavInfo.Name;

                                            if (!File.Exists(type.MlvStoreDir + Path.DirectorySeparatorChar + mlvWavInfo.Name))
                                            {
                                                AddFileInfo(xml, mlvWavInfo, mlvWavInfo.FullName);
                                            }
                                        }
                                    }

                                    /* add one file per frame */
                                    foreach (uint frame in MLVAccessor.GetFrameNumbers(mlvFileInfo.FullName))
                                    {
                                        if (Program.Instance.Server.Settings.ShowDng)
                                        {
                                            string frameName = frame.ToString("000000") + ".dng";
                                            DAVFileDirInfo frameInfo = new DAVFileDirInfo(type.MlvBaseName + "_" + frameName);

                                            /* one DNG file per frame */
                                            frameInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.Length = MLVAccessor.GetFileSize(mlvFileInfo.FullName, frameName);
                                            frameInfo.FullName = RequestPath + "/" + frameInfo.Name;
                                            frameInfo.DisplayName = "Frame " + frame + " as DNG";

                                            if (!File.Exists(type.MlvStoreDir + Path.DirectorySeparatorChar + frameInfo.Name))
                                            {
                                                AddFileInfo(xml, frameInfo, frameInfo.FullName);
                                            }
                                        }

                                        /* one JPEG file per frame if enabled */
                                        if (Program.Instance.Server.Settings.ShowJpeg)
                                        {
                                            string frameName = frame.ToString("000000") + ".jpg";
                                            DAVFileDirInfo frameInfo = new DAVFileDirInfo(type.MlvBaseName + "_" + frameName);

                                            frameInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.Length = MLVAccessor.GetFileSize(mlvFileInfo.FullName, frameName);
                                            frameInfo.FullName = RequestPath + "/" + frameInfo.Name;
                                            frameInfo.DisplayName = "Frame " + frame + " as Jpeg";

                                            if (!File.Exists(type.MlvStoreDir + Path.DirectorySeparatorChar + frameInfo.Name))
                                            {
                                                AddFileInfo(xml, frameInfo, frameInfo.FullName);
                                            }
                                        }

                                        /* one FITS file per frame if enabled */
                                        if (Program.Instance.Server.Settings.ShowFits)
                                        {
                                            string frameName = frame.ToString("000000") + ".fits";
                                            DAVFileDirInfo frameInfo = new DAVFileDirInfo(type.MlvBaseName + "_" + frameName);

                                            frameInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(mlvFileInfo.FullName, frameName);
                                            frameInfo.Length = MLVAccessor.GetFileSize(mlvFileInfo.FullName, frameName);
                                            frameInfo.FullName = RequestPath + "/" + frameInfo.Name;
                                            frameInfo.DisplayName = "Frame " + frame + " as FITS";

                                            if (!File.Exists(type.MlvStoreDir + Path.DirectorySeparatorChar + frameInfo.Name))
                                            {
                                                AddFileInfo(xml, frameInfo, frameInfo.FullName);
                                            }
                                        }
                                    }

                                    /* add locally stored files */
                                    if (Directory.Exists(type.MlvStoreDir))
                                    {
                                        AddFilesAndFolders(xml, type.MlvStoreDir, depth - 1);
                                    }
                                }

                                xml.WriteEndElement();
                                xml.WriteEndDocument();
                                xml.Flush();

                                StatusCode = GetStatusCode(207);
                                ResponseTextContent = RequestHandler.StreamToString(mem);

                                Server.LogRequest("Providing file list for MLV file: " + type.MlvFilePath);
                            }
                            catch(FileNotFoundException e)
                            {
                                Server.Log("[E] Invalid MLV location: " + type.RequestPath);
                                StatusCode = GetStatusCode(404);
                            }
                            catch(Exception e)
                            {
                                Server.Log("[E] Error while browsing " + type.RequestPath + ": " + e.GetType() + ", " + e.Message);
                                StatusCode = GetStatusCode(500);
                            }
                        }
                        break;

                    case FileType.PureVirtualFile:
                        {
                            XmlTextWriter xml = new XmlTextWriter(mem, Encoding.UTF8);

                            xml.Formatting = Formatting.Indented;
                            xml.IndentChar = ' ';
                            xml.Indentation = 1;
                            xml.WriteStartDocument();

                            //Set the Multistatus
                            xml.WriteStartElement("D", "multistatus", "DAV:");
                            DAVFileDirInfo mlvItemInfo = new DAVFileDirInfo(type.MlvFileContent);

                            mlvItemInfo.CreationTimeUtc = MLVAccessor.GetFileDateUtc(type.MlvFilePath, type.MlvFileContent);
                            mlvItemInfo.LastWriteTimeUtc = MLVAccessor.GetFileDateUtc(type.MlvFilePath, type.MlvFileContent);
                            mlvItemInfo.Length = MLVAccessor.GetFileSize(type.MlvFilePath, type.MlvFileContent);
                            mlvItemInfo.FullName = RequestPath + "/" + mlvItemInfo.Name;

                            AddFileInfo(xml, mlvItemInfo, RequestPath);

                            xml.WriteEndElement();
                            xml.WriteEndDocument();
                            xml.Flush();

                            StatusCode = GetStatusCode(207);
                            ResponseTextContent = RequestHandler.StreamToString(mem);
                            MimeType = "text/xml; charset=\"utf-8\"";
                        }
                        break;

                    case FileType.Redirected:
                    case FileType.Direct:
                        if (Directory.Exists(type.LocalPath))
                        {
                            XmlTextWriter xml = new XmlTextWriter(mem, Encoding.UTF8);

                            xml.Formatting = Formatting.Indented;
                            xml.IndentChar = ' ';
                            xml.Indentation = 1;
                            xml.WriteStartDocument();

                            //Set the Multistatus
                            xml.WriteStartElement("D", "multistatus", "DAV:");

                            AddDirectoryInfo(xml, new DirectoryInfo(type.LocalPath), RequestPath);

                            int depth = 0;
                            if (DepthHeaderLine == "1")
                            {
                                depth = 1;
                            }
                            else if (DepthHeaderLine == "infinity")
                            {
                                depth = int.MaxValue;
                            }

                            AddFilesAndFolders(xml, type.LocalPath, depth - 1);

                            xml.WriteEndElement();
                            xml.WriteEndDocument();
                            xml.Flush();

                            StatusCode = GetStatusCode(207);
                            ResponseTextContent = RequestHandler.StreamToString(mem);

                            Server.LogRequest("Providing file list for " + type.LocalPath);
                        }
                        else if (File.Exists(type.LocalPath))
                        {
                            XmlTextWriter xml = new XmlTextWriter(mem, Encoding.UTF8);

                            xml.Formatting = Formatting.Indented;
                            xml.IndentChar = ' ';
                            xml.Indentation = 1;
                            xml.WriteStartDocument();

                            //Set the Multistatus
                            xml.WriteStartElement("D", "multistatus", "DAV:");

                            AddFileInfo(xml, new FileInfo(type.LocalPath), RequestPath);

                            xml.WriteEndElement();
                            xml.WriteEndDocument();
                            xml.Flush();

                            StatusCode = GetStatusCode(207);
                            ResponseTextContent = RequestHandler.StreamToString(mem);
                            MimeType = "text/xml; charset=\"utf-8\"";

                            Server.LogRequest("Providing file info for " + type.LocalPath);
                        }
                        else
                        {
                            StatusCode = GetStatusCode(404);
                        }
                        break;
                }
            }

            base.HandleRequest(stream);
        }

        private string ValidateFileName(string file)
        {
            return Path.GetInvalidFileNameChars().Aggregate(file, (current, c) => current.Replace(c.ToString(), "_"));
        }

        private void AddFilesAndFolders(XmlTextWriter xml, string localName, int depth)
        {
            if (depth < 0)
            {
                return;
            }

            foreach (string dirname in Directory.GetDirectories(localName))
            {
                DirectoryInfo info = new DirectoryInfo(dirname);

                if (info.Name.ToUpper().EndsWith(".MLD") || info.Name.ToUpper().EndsWith(".RAD"))
                {
                }
                else
                {
                    AddDirectoryInfo(xml, info, (RequestPath + "/" + info.Name).Replace("//", "/"));
                    AddFilesAndFolders(xml, localName + "\\" + info.Name, depth - 1);
                }
            }

            foreach (string filename in Directory.GetFiles(localName))
            {
                FileInfo info = new FileInfo(filename);

                if (info.Name.ToUpper().EndsWith(".MLV") || info.Name.ToUpper().EndsWith(".RAW"))
                {
                    AddDirectoryInfo(xml, null, (RequestPath + "/" + info.Name).Replace("//", "/"));
                }
                else
                {
                    AddFileInfo(xml, info, (RequestPath + "/" + info.Name).Replace("//", "/"));
                }
            }
        }

        class DAVFileDirInfo
        {
            private FileInfo fileInfo = null;
            private DirectoryInfo dirInfo = null;

            public DateTime CreationTimeUtc;
            public DateTime LastWriteTimeUtc;
            public string Name;
            public string FullName;
            public long Length;
            public string DisplayName;

            public DAVFileDirInfo(string file)
            {
                Name = file;
            }

            public DAVFileDirInfo(FileInfo file)
            {
                fileInfo = file;

                CreationTimeUtc = fileInfo.CreationTimeUtc;
                LastWriteTimeUtc = fileInfo.LastWriteTimeUtc;
                Name = fileInfo.Name;
                FullName = fileInfo.FullName;
                if (fileInfo.Exists)
                {
                    Length = fileInfo.Length;
                }
            }

            public DAVFileDirInfo(DirectoryInfo dir)
            {
                dirInfo = dir;

                CreationTimeUtc = dirInfo.CreationTimeUtc;
                LastWriteTimeUtc = dirInfo.LastWriteTimeUtc;
                Name = dirInfo.Name;
                FullName = dirInfo.FullName;
                Length = 0;
            }

            public static implicit operator DAVFileDirInfo(FileInfo info)
            {
                return new DAVFileDirInfo(info);
            }

            public static implicit operator DAVFileDirInfo(DirectoryInfo info)
            {
                return new DAVFileDirInfo(info);
            }
        }

        private long GetTotalFreeSpace(string path)
        {
            string driveName = Path.GetPathRoot(path);

            foreach (DriveInfo drive in DriveInfo.GetDrives())
            {
                if (drive.IsReady && drive.Name == driveName)
                {
                    return drive.TotalFreeSpace;
                }
            }
            return -1;
        }

        private long GetTotalUsedSpace(string path)
        {
            string driveName = Path.GetPathRoot(path);

            foreach (DriveInfo drive in DriveInfo.GetDrives())
            {
                if (drive.IsReady && drive.Name == driveName)
                {
                    return drive.TotalSize - drive.TotalFreeSpace;
                }
            }
            return -1;
        }

        private void AddDirectoryInfo(XmlTextWriter xml, DAVFileDirInfo info, string item)
        {
            xml.WriteStartElement("response", "DAV:");
            {
                xml.WriteElementString("href", "DAV:", BaseHref + EscapeSpecialChars(item));
                xml.WriteStartElement("propstat", "DAV:");
                {
                    xml.WriteStartElement("prop", "DAV:");
                    {
                        if (info != null)
                        {
                            xml.WriteElementString("creationdate", "DAV:", info.CreationTimeUtc.ToString("r", CultureInfo.InvariantCulture));
                            xml.WriteElementString("getlastmodified", "DAV:", info.LastWriteTimeUtc.ToString("r", CultureInfo.InvariantCulture));
                            //xml.WriteElementString("quota-available-bytes", "DAV:", GetTotalFreeSpace(info.FullName).ToString());
                            //xml.WriteElementString("quota-used-bytes", "DAV:", GetTotalUsedSpace(info.FullName).ToString());
                        }
                        
                        xml.WriteStartElement("resourcetype", "DAV:");
                        {
                            xml.WriteElementString("collection", "DAV:","");
                        }
                        xml.WriteEndElement();
                    }
                    xml.WriteEndElement();

                    xml.WriteElementString("status", "DAV:", GetStatusCode(200));
                }
                xml.WriteEndElement();
            }
            xml.WriteEndElement();

#if false
            
            xml.WriteStartElement("response", "DAV:");

            //Load the valid items HTTP/1.1 200 OK
            xml.WriteElementString("href", "DAV:", EscapeSpecialChars(href));

            //Open the propstat element section
            xml.WriteStartElement("propstat", "DAV:");
            xml.WriteElementString("status", "DAV:", GetStatusCode(200));

            //Open the prop element section
            xml.WriteStartElement("prop", "DAV:");

            xml.WriteElementString("contenttype", "DAV:", "application/webdav-collection");

            xml.WriteStartElement("lastmodified", "DAV:");
            xml.WriteAttributeString("b:dt", "dateTime.rfc1123");
            xml.WriteString(DateTime.Now.ToUniversalTime().ToString("r", CultureInfo.InvariantCulture));
            xml.WriteEndElement();

            xml.WriteElementString("contentlength", "DAV:", "0");

            xml.WriteStartElement("resourcetype", "DAV:");
            xml.WriteElementString("collection", "DAV:", "");
            xml.WriteEndElement();

            //Close the prop element section
            xml.WriteEndElement();

            //Close the propstat element section
            xml.WriteEndElement();
            xml.WriteEndElement();
#endif
        }


        private void AddFileInfo(XmlTextWriter xml, DAVFileDirInfo info, string item)
        {
            xml.WriteStartElement("response", "DAV:");
            {
                xml.WriteElementString("href", "DAV:", BaseHref + EscapeSpecialChars(item));
                xml.WriteStartElement("propstat", "DAV:");
                {
                    xml.WriteStartElement("prop", "DAV:");
                    {
                        if (info != null)
                        {
                            xml.WriteElementString("creationdate", "DAV:", info.CreationTimeUtc.ToString("r", CultureInfo.InvariantCulture));
                            xml.WriteElementString("getlastmodified", "DAV:", info.LastWriteTimeUtc.ToString("r", CultureInfo.InvariantCulture));
                            xml.WriteElementString("getcontenttype", "DAV:", GetMimeType(info.FullName));
                            xml.WriteElementString("getcontentlength", "DAV:", "" + Math.Min(info.Length, Int32.MaxValue));
                            xml.WriteElementString("getetag", "DAV:", "1/" + BaseHref + EscapeSpecialChars(item));
                            xml.WriteElementString("displayname", "DAV:", info.DisplayName);
                        }
                        xml.WriteElementString("getresourcetype", "DAV:", "");
                        xml.WriteElementString("resourcetype", "DAV:", "");
                        xml.WriteElementString("supportedlock", "DAV:", "");
                    }
                    xml.WriteEndElement();

                    xml.WriteElementString("status", "DAV:", GetStatusCode(200));
                }
                xml.WriteEndElement();
            }
            xml.WriteEndElement();



#if false
            xml.WriteStartElement("response", "DAV:");

            //Load the valid items HTTP/1.1 200 OK
            xml.WriteElementString("href", "DAV:", EscapeSpecialChars(href));

            //Open the propstat element section
            xml.WriteStartElement("propstat", "DAV:");
            xml.WriteElementString("status", "DAV:", GetStatusCode(200));

            //Open the prop element section
            xml.WriteStartElement("prop", "DAV:");

            xml.WriteElementString("getcontenttype", "DAV:", GetMimeType(info.FullName));

            xml.WriteStartElement("getlastmodified", "DAV:");
            xml.WriteAttributeString("b:dt", "dateTime.rfc1123");
            xml.WriteString(DateTime.Now.ToUniversalTime().ToString("r", CultureInfo.InvariantCulture));
            xml.WriteEndElement();

            xml.WriteElementString("getcontentlength", "DAV:", "" + Math.Min(info.Length, Int32.MaxValue));
            xml.WriteElementString("getresourcetype", "DAV:", "");

            xml.WriteElementString("getetag", "DAV:", "\"" + info.GetHashCode() + "\"");

            //Close the prop element section
            xml.WriteEndElement();

            //Close the propstat element section
            xml.WriteEndElement();
            xml.WriteEndElement();
#endif
        }
    }

    public class CopyHandler : RequestHandler
    {
        public CopyHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            string dstLocation = GetDestinationLocation();
            VirtualFileType srcType = VirtualFileType.Resolve(RequestPath);
            VirtualFileType dstType = VirtualFileType.Resolve(dstLocation);
            string srcPath = srcType.LocalPath;
            string dstPath = dstType.LocalPath;

            switch (srcType.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    base.HandleRequest(stream);
                    return;

                case FileType.PureVirtualDir:
                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(403);
                    base.HandleRequest(stream);
                    return;
            }

            switch (srcType.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    base.HandleRequest(stream);
                    return;

                case FileType.PureVirtualDir:
                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(403);
                    base.HandleRequest(stream);
                    return;
            }

            bool success = false;

            if (srcPath != null && dstPath != null)
            {
                if (File.Exists(srcPath))
                {
                    try
                    {
                        File.Copy(srcPath, dstPath);
                        success = true;
                    }
                    catch (Exception e)
                    {
                        success = false;
                    }
                }
                else if (Directory.Exists(srcPath))
                {
                    try
                    {
                        CopyDirectory(srcPath, dstPath);
                        success = true;
                    }
                    catch (Exception e)
                    {
                        success = false;
                    }
                }
            }

            if (!success)
            {
                StatusCode = GetStatusCode(403);
            }
            else
            {
                StatusCode = GetStatusCode(201);
            }

            MimeType = "text/plain";

            base.HandleRequest(stream);
        }

        public static void CopyDirectory(string srcDir, string dstDir)
        {
            if (dstDir[dstDir.Length - 1] != Path.DirectorySeparatorChar)
            {
                dstDir += Path.DirectorySeparatorChar;
            }

            if (!Directory.Exists(dstDir))
            {
                Directory.CreateDirectory(dstDir);
            }

            String[] entries = Directory.GetFileSystemEntries(srcDir);

            foreach (string entry in entries)
            {
                if (Directory.Exists(entry))
                {
                    CopyDirectory(entry, dstDir + Path.GetFileName(entry));
                }
                else
                {
                    File.Copy(entry, dstDir + Path.GetFileName(entry), true);
                }
            }
        }
    }

    public class MoveHandler : RequestHandler
    {
        public MoveHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            string dstLocation = GetDestinationLocation();
            VirtualFileType srcType = VirtualFileType.Resolve(RequestPath);
            VirtualFileType dstType = VirtualFileType.Resolve(dstLocation);
            string srcPath = srcType.LocalPath;
            string dstPath = dstType.LocalPath;

            switch (srcType.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    base.HandleRequest(stream);
                    return;

                case FileType.PureVirtualDir:
                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(403);
                    base.HandleRequest(stream);
                    return;
            }

            switch (srcType.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    base.HandleRequest(stream);
                    return;

                case FileType.PureVirtualDir:
                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(403);
                    base.HandleRequest(stream);
                    return;
            }

            bool success = false;

            if (srcPath != null && dstPath != null)
            {
                if (File.Exists(srcPath))
                {
                    try
                    {
                        File.Move(srcPath, dstPath);
                        success = true;
                    }
                    catch (Exception e)
                    {
                        success = false;
                    }
                }
                else if (Directory.Exists(srcPath))
                {
                    try
                    {
                        Directory.Move(srcPath, dstPath);
                        success = true;
                    }
                    catch (Exception e)
                    {
                        success = false;
                    }
                }
            }

            if (!success)
            {
                StatusCode = GetStatusCode(403);
            }
            else
            {
                StatusCode = GetStatusCode(201);
            }

            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }

    public class DeleteHandler : RequestHandler
    {
        public DeleteHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }
            
            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    break;

                case FileType.PureVirtualDir:
                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(403);
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    {
                        bool success = false;

                        if (File.Exists(type.LocalPath))
                        {
                            try
                            {
                                File.Delete(type.LocalPath);
                                success = true;
                            }
                            catch (Exception e)
                            {
                                success = false;
                            }
                        }
                        else if (Directory.Exists(type.LocalPath))
                        {
                            try
                            {
                                Directory.Delete(type.LocalPath, true);
                                success = true;
                            }
                            catch (Exception e)
                            {
                                success = false;
                            }
                        }

                        if (!success)
                        {
                            StatusCode = GetStatusCode(403);
                        }
                        else
                        {
                            StatusCode = GetStatusCode(204);
                        }

                        MimeType = "text/plain";
                    }
                    break;
            }

            base.HandleRequest(stream);
        }
    }

    public class PutHandler : RequestHandler
    {
        /* creating a binary reader stream in HandleContent */
        private BinaryReader PutDataStream = null;

        public PutHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleContent(Stream stream)
        {
            /* reading data later while writing the file */
            PutDataStream = new BinaryReader(stream);
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }
            
            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            /* writes to existing files get redirected into virtual dir */
            if (type.Type == FileType.PureVirtualFile)
            {
                type.Type = FileType.Redirected;
            }

            switch (type.Type)
            {
                case FileType.PureVirtualDir:
                    StatusCode = GetStatusCode(400);
                    break;

                case FileType.PureVirtualFile:
                    StatusCode = GetStatusCode(500);
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    {
                        bool existed = false;
                        bool success = false;

                        if (File.Exists(type.LocalPath))
                        {
                            existed = true;
                        }

                        string dir = new FileInfo(type.LocalPath).DirectoryName;
                        if(!Directory.Exists(dir))
                        {
                            Directory.CreateDirectory(dir);
                        }

                        /* allowed to write, so start reading from input stream and write to disk */
                        FileStream file = null;
                        WebDAVServer.TransferInfo info = new WebDAVServer.TransferInfo(Server, RequestContentLength);

                        try
                        {
                            int blockSize = 4096;
                            long remaining = RequestContentLength;
                            byte[] readBuffer = new byte[blockSize];

                            file = File.Open(type.LocalPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Write);

                            file.Lock(0, file.Length);

                            if (file != null)
                            {
                                while (remaining > 0)
                                {
                                    int length = (int)Math.Min(remaining, blockSize);

                                    length = PutDataStream.Read(readBuffer, 0, length);
                                    if (length > 0)
                                    {
                                        file.Write(readBuffer, 0, length);

                                        remaining -= length;
                                        BytesReadData += length;
                                        info.BlockTransferred(length);
                                    }
                                    else
                                    {
                                        Server.Log("[E] read <0 byte during PUT request");
                                        remaining = 0;
                                    }
                                }

                                file.Unlock(0, file.Length);
                                file.Close();
                                file = null;
                                success = true;
                            }
                        }
                        catch (Exception e)
                        {
                            Server.Log("[E] "+e.ToString()+" during PUT request");
                            success = false;
                        }

                        info.TransferFinished();

                        if (!success)
                        {
                            StatusCode = GetStatusCode(403);
                        }
                        else if (existed)
                        {
                            StatusCode = GetStatusCode(200);
                        }
                        else
                        {
                            StatusCode = GetStatusCode(201);
                        }

                        MimeType = "text/plain";
                    }
                    break;
            }

            base.HandleRequest(stream);
        }
    }

    public class GetHandler : RequestHandler
    {
        public GetHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch(type.Type)
            {
                case FileType.Invalid:
                case FileType.PureVirtualDir:
                    StatusCode = GetStatusCode(400);
                    break;

                case FileType.PureVirtualFile:
                    {
                        try
                        {
                            byte[] returnStream = MLVAccessor.GetDataStream(type.MlvFilePath, type.MlvFileContent);

                            ResponseBinaryContent = returnStream;
                            ResponseBinaryContentLength = returnStream.Length;

                            StatusCode = GetStatusCode(200);
                            HeaderLines.Add("ETag: 1/" + RequestPath);
                            HeaderLines.Add("Vary: Accept-Encoding");
                            HeaderLines.Add("Accept-Ranges: bytes");
                            MimeType = GetMimeType(type.MlvFileContent);
                        }
                        catch (FileNotFoundException e)
                        {
                            Server.Log("[E] Invalid MLV location: " + type.RequestPath);
                            StatusCode = GetStatusCode(404);
                        }
                        catch (Exception e)
                        {
                            Server.Log("[E] Error while browsing " + type.RequestPath + ": " + e.GetType() + ", " + e.Message);
                            StatusCode = GetStatusCode(500);
                        }
                    }
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    {
                        if (RequestPath.StartsWith("/log"))
                        {
                            SkipRequestLogging = true;
                            LogRequest(RequestPath);
                        }
                        else if (RequestPath.StartsWith("/debug"))
                        {
                            SkipRequestLogging = true;
                            DebugRequest(RequestPath);
                        }
                        else if (File.Exists(type.LocalPath))
                        {
                            try
                            {
                                FileStream file = File.Open(type.LocalPath, FileMode.Open, FileAccess.Read, FileShare.Read);

                                byte[] data = new byte[file.Length];
                                file.Read(data, 0, data.Length);
                                file.Close();

                                ResponseBinaryContent = data;
                                ResponseBinaryContentLength = data.Length;

                                StatusCode = GetStatusCode(200);
                                HeaderLines.Add("ETag: 1/" + RequestPath);
                                HeaderLines.Add("Vary: Accept-Encoding");
                                HeaderLines.Add("Accept-Ranges: bytes");
                                MimeType = GetMimeType(type.LocalPath);
                            }
                            catch (FileNotFoundException e)
                            {
                                StatusCode = GetStatusCode(404);
                            }
                            catch (Exception e)
                            {
                                StatusCode = GetStatusCode(500);
                            }
                        }
                        else if (Directory.Exists(type.LocalPath))
                        {
                            Server.LogRequest("Listing directory for " + type.LocalPath);
                            StatusCode = GetStatusCode(200);

                            string listing = "<html>";
                            string pathUp = CleanPath(RequestPath + "/..");

                            listing += "<head><title>Index of " + RequestPath + "</title></head>" + Environment.NewLine;
                            listing += "<body>";
                            listing += "<h1>Index of " + RequestPath + "</h1>" + Environment.NewLine;
                            AddHeader(ref listing);
                            listing += "<hr><pre>";

                            /* don't show .. if we are at top level already */
                            if (RequestPath != "/")
                            {
                                listing += "<a href=\"/" + pathUp + "\">../</a>" + Environment.NewLine;
                            }

                            /* now dump all files/dirs */
                            string path = GetLocalName(RootPath, RequestPath);
                            foreach (string dirname in Directory.GetDirectories(path))
                            {
                                DirectoryInfo info = new DirectoryInfo(dirname);
                                string url = (RequestPath + "/" + info.Name).Replace("//", "/");
                                url = /*Uri.EscapeUriString*/EscapeSpecialChars(url);

                                listing += "<a href=\"" + url + "\">" + info.Name + "</a>" + Environment.NewLine;
                            }

                            foreach (string filename in Directory.GetFiles(path))
                            {
                                FileInfo info = new FileInfo(filename);
                                string url = (RequestPath + "/" + info.Name).Replace("//", "/");
                                url = /*Uri.EscapeUriString*/EscapeSpecialChars(url);

                                listing += "<a href=\"" + url + "\">" + info.Name + "</a>" + Environment.NewLine;
                            }
                            listing += "</pre>";
                            AddFooter(ref listing);
                            listing += "</body></html>";

                            ResponseTextContent = listing;
                            MimeType = "text/html";
                        }
                        else
                        {
                            StatusCode = GetStatusCode(404);
                        }
                    }
                    break;
            }

            base.HandleRequest(stream);
        }


        private void DebugRequest(string request)
        {
            string listing = "<html>";
            listing += "<head><title>Info</title></head>" + Environment.NewLine;
            listing += "<body bgcolor=\"white\">";
            listing += "<h1>Info</h1>" + Environment.NewLine;
            AddHeader(ref listing);
            listing += "<hr><pre>";
            listing += "Config file path:     " + Server.ConfigFilePath + Environment.NewLine;
            listing += "Config file name:     " + Server.ConfigFileName + Environment.NewLine;
            listing += "Config file status:   " + Server.ConfigFileStatus + Environment.NewLine;
            listing += "" + Environment.NewLine;
            listing += "MachineName:          " + Environment.MachineName + Environment.NewLine;
            listing += "UserName:             " + Environment.UserName + Environment.NewLine;
            listing += "OSVersion:            " + Environment.OSVersion + Environment.NewLine;
            listing += "ProcessorCount:       " + Environment.ProcessorCount + Environment.NewLine;
            listing += "CurrentDirectory:     " + Environment.CurrentDirectory + Environment.NewLine;
            listing += "SystemDirectory:      " + Environment.SystemDirectory + Environment.NewLine;
            listing += "Version:              " + Environment.Version + Environment.NewLine;

            if (AuthTokens == null || AuthTokens == "")
            {
                listing += "CommandLine:          " + Environment.CommandLine + Environment.NewLine;
                listing += "EnvironmentVariables: " + Environment.NewLine;
                foreach (DictionaryEntry var in Environment.GetEnvironmentVariables())
                {
                    listing += string.Format("      {0} = {1}", var.Key, var.Value) + Environment.NewLine;
                }
            }
            else
            {
                listing += "CommandLine:          (Feature disabled. Disable authentication to use this feature.)" + Environment.NewLine;
                listing += "EnvironmentVariables: (Feature disabled. Disable authentication to use this feature." + Environment.NewLine;
            }

            listing += "</pre>" + Environment.NewLine;
            AddFooter(ref listing);

            ResponseTextContent = listing;
            MimeType = "text/html";
            StatusCode = GetStatusCode(200);
        }

        private void LogRequest(string request)
        {
            string command = "";

            if (request.Contains('?'))
            {
                command = request.Split('?')[1];
            }

            switch (command)
            {
                case "log_clear":
                    Server.LogMessages = "";
                    break;
                case "req_clear":
                    Server.RequestMessages = "";
                    break;
                case "req_start":
                    Server.EnableRequestLog = true;
                    break;
                case "req_stop":
                    Server.EnableRequestLog = false;
                    break;
            }

            if (command != "")
            {
                HeaderLines.Add("Location: /log");
                StatusCode = GetStatusCode(302);
                return;
            }

            StatusCode = GetStatusCode(200);

            string listing = "<html>";

            lock (Server.StatisticsLock)
            {
                listing += "<head><title>Log messages</title></head>" + Environment.NewLine;
                listing += "<body bgcolor=\"white\">";
                listing += "<h1>Log</h1>" + Environment.NewLine;
                AddHeader(ref listing);
                listing += "<hr>";
                listing += "<h3>Statistics</h1>" + Environment.NewLine;
                listing += "<pre>";

                listing += " --------------------------------------" + Environment.NewLine;
                listing += " Connections    | Total:  [ " + string.Format("{0,8}", Server.Connections) + " ]" + Environment.NewLine;
                listing += " ---------------|----------------------" + Environment.NewLine;
                listing += " Bytes read     | Total:  [ " + string.Format("{0,8}", Server.BytesReadData + Server.BytesReadHeader) + " ]" + Environment.NewLine;
                listing += "                | Data:   [ " + string.Format("{0,8}", Server.BytesReadData) + " ]" + Environment.NewLine;
                listing += " ---------------|----------------------" + Environment.NewLine;
                listing += " Bytes written  | Total:  [ " + string.Format("{0,8}", Server.BytesWrittenData + Server.BytesWrittenHeader) + " ]" + Environment.NewLine;
                listing += "                | Data:   [ " + string.Format("{0,8}", Server.BytesWrittenData) + " ]" + Environment.NewLine;
                listing += " --------------------------------------" + Environment.NewLine;
                listing += "</pre><hr>";
            }

            listing += "<h3>Log messages</h1><hr><pre>" + Environment.NewLine;
            listing += "<pre><a href=\"/log?log_clear\">[Clear]</a></pre>";
            listing += Server.LogMessages;

            listing += "</pre><h3>Request messages</h1><hr><pre>" + Environment.NewLine;

            if (AuthTokens == null || AuthTokens == "")
            {
                if (!Server.EnableRequestLog)
                {
                    listing += "<pre><a href=\"/log?req_start\">[Start]</a> | <a href=\"/log?req_clear\">[Clear]</a></pre>";
                    listing += "------------------------------" + Environment.NewLine;
                    listing += "-- Request logging disabled --" + Environment.NewLine;
                    listing += "------------------------------" + Environment.NewLine;
                }
                else
                {
                    listing += "<pre><a href=\"/log?req_stop\">[Stop]</a> | <a href=\"/log?req_clear\">[Clear]</a></pre>";
                    listing += Server.RequestMessages;
                }
            }
            else
            {
                listing += "(Feature disabled. Disable authentication to use this feature.)" + Environment.NewLine;
            }

            listing += "</pre>" + Environment.NewLine;
            AddFooter(ref listing);

            ResponseTextContent = listing;
            MimeType = "text/html";
        }
    }


    public class HeadHandler : GetHandler
    {
        public HeadHandler(WebDAVServer server, string request)
            : base(server, request)
        {
            SendHeaderOnly = true;
        }

        public override void HandleRequest(Stream stream)
        {
            base.HandleRequest(stream);
        }

    }

    public class OptionsHandler : RequestHandler
    {
        public OptionsHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }
            
            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    break;

                case FileType.PureVirtualFile:
                case FileType.PureVirtualDir:
                    StatusCode = GetStatusCode(200);
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    if (File.Exists(type.LocalPath))
                    {
                        StatusCode = GetStatusCode(200);
                    }
                    else if (Directory.Exists(type.LocalPath))
                    {
                        StatusCode = GetStatusCode(200);
                    }
                    else
                    {
                        StatusCode = GetStatusCode(400);
                    }
                    break;
            }

            AddDavHeader();
            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }


    public class LockHandler : RequestHandler
    {
        public LockHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            bool found = false;
            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    break;

                case FileType.PureVirtualFile:
                case FileType.PureVirtualDir:
                    found = true;
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    if (File.Exists(type.LocalPath))
                    {
                        found = true;
                    }
                    else if (Directory.Exists(type.LocalPath))
                    {
                        found = true;
                    }
                    break;
            }

            if (found)
            {
                /* this is simply saying "yes, you got the lock, mr. x" without any checks.
                 * it even returns dummy tokens and names and doesnt check what the requester really wants.
                 * looks like it is fine ;)
                 */
                using (Stream mem = new MemoryStream())
                {
                    XmlTextWriter xml = new XmlTextWriter(mem, Encoding.UTF8);

                    xml.Formatting = Formatting.Indented;
                    xml.IndentChar = ' ';
                    xml.Indentation = 1;
                    xml.WriteStartDocument();

                    xml.WriteStartElement("D", "prop ", "DAV:");

                    xml.WriteStartElement("lockdiscovery", "DAV:");
                    {
                        xml.WriteStartElement("activelock", "DAV:");
                        {
                            xml.WriteStartElement("locktype", "DAV:");
                            {
                                xml.WriteElementString("write", "DAV:", "");
                            }
                            xml.WriteEndElement();

                            xml.WriteStartElement("lockscope", "DAV:");
                            {
                                xml.WriteElementString("exclusive", "DAV:", "");
                            }
                            xml.WriteEndElement();

                            xml.WriteStartElement("owner", "DAV:");
                            {
                                xml.WriteElementString("href", "DAV:", "you");
                            }
                            xml.WriteEndElement();

                            xml.WriteStartElement("locktoken", "DAV:");
                            {
                                xml.WriteElementString("href", "DAV:", "token");
                            }
                            xml.WriteEndElement();

                            xml.WriteElementString("depth", "DAV:", "Infinity");
                            xml.WriteElementString("timeout", "DAV:", "Second-604800");
                        }
                        xml.WriteEndElement();
                    }
                    xml.WriteEndElement();

                    xml.WriteEndDocument();
                    xml.Flush();

                    StatusCode = GetStatusCode(200);
                    ResponseTextContent = RequestHandler.StreamToString(mem);
                    MimeType = "text/xml; charset=\"utf-8\"";
                }
            }
            else
            {
                StatusCode = GetStatusCode(404);
            }

            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }


    public class UnlockHandler : RequestHandler
    {
        public UnlockHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            bool found = false;
            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    break;

                case FileType.PureVirtualFile:
                case FileType.PureVirtualDir:
                    found = true;
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    if (File.Exists(type.LocalPath))
                    {
                        found = true;
                    }
                    else if (Directory.Exists(type.LocalPath))
                    {
                        found = true;
                    }
                    break;
            }

            if (found)
            {
                StatusCode = GetStatusCode(204);
            }
            else
            {
                StatusCode = GetStatusCode(404);
            }

            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }


    public class MkColHandler : RequestHandler
    {
        public MkColHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(403);
                    break;

                case FileType.PureVirtualFile:
                case FileType.PureVirtualDir:
                    StatusCode = GetStatusCode(403);
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    if (File.Exists(type.LocalPath) || Directory.Exists(type.LocalPath))
                    {
                        StatusCode = GetStatusCode(409);
                    }
                    else
                    {
                        DirectoryInfo info = null;

                        try
                        {
                            info = Directory.CreateDirectory(type.LocalPath);
                        }
                        catch (Exception)
                        {
                        }

                        if (info != null)
                        {
                            StatusCode = GetStatusCode(201);
                        }
                        else
                        {
                            StatusCode = GetStatusCode(507);
                        }
                    }
                    break;
            }

            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }

    public class PropPatchHandler : RequestHandler
    {
        public PropPatchHandler(WebDAVServer server, string request)
            : base(server, request)
        {
        }

        public override void HandleRequest(Stream stream)
        {
            if (IsAccessDenied())
            {
                base.HandleRequest(stream);
                return;
            }

            VirtualFileType type = VirtualFileType.Resolve(RequestPath);

            switch (type.Type)
            {
                case FileType.Invalid:
                    StatusCode = GetStatusCode(404);
                    break;

                case FileType.PureVirtualFile:
                case FileType.PureVirtualDir:
                    StatusCode = GetStatusCode(403);
                    break;

                case FileType.Redirected:
                case FileType.Direct:
                    if (File.Exists(type.LocalPath) || Directory.Exists(type.LocalPath))
                    {
                        StatusCode = GetStatusCode(200);
                    }
                    else
                    {
                        StatusCode = GetStatusCode(404);
                    }
                    break;
            }


            MimeType = "text/plain";

            base.HandleRequest(stream);
        }
    }
}
