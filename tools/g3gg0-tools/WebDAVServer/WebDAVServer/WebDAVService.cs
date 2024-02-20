using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ServiceProcess;

namespace WebDAVServer
{
    class WebDAVService : ServiceBase
    {
        public WebDAVServer Server = null;
        public static string WebDAVServiceName = "WebDAVServer";
        public static string WebDAVDisplayName = "WebDAVServer v" + WebDAVServer.Version + " by g3gg0.de";

        public WebDAVService()
        {
            ServiceName = WebDAVServiceName;
            EventLog.Log = "Application";
            CanShutdown = true;
            CanStop = true;
        }

        public static bool Installed
        {
            get
            {
                try
                {
                    ServiceController ctl = ServiceController.GetServices().FirstOrDefault(s => s.ServiceName == "WebDAVServiceName");

                    if(ctl == null)
                    {
                        return false;
                    }

                    ServiceController sc = new ServiceController();
                    sc.ServiceName = WebDAVServiceName;
                    string status = sc.Status.ToString().ToLower();

                    if (status == "stopped")
                    {
                        return true;
                    }

                    if (status == "running" || status == "startpending")
                    {
                        return true;
                    }
                }
                catch
                {
                }

                return false;
            }
        }

        public static bool Running
        {
            get
            {
                try
                {
                    ServiceController sc = new ServiceController();
                    sc.ServiceName = WebDAVServiceName;
                    string status = sc.Status.ToString().ToLower();

                    if (status == "stopped")
                    {
                        return false;
                    }

                    if (status == "running" || status == "startpending")
                    {
                        return true;
                    }
                }
                catch
                {
                }

                return false;
            }
        }

        public static void StartService()
        {
            try
            {
                ServiceController sc = new ServiceController();
                sc.ServiceName = WebDAVServiceName;
                sc.Start();
            }
            catch
            {
            }
        }

        public static void StopService()
        {
            try
            {
                ServiceController sc = new ServiceController();
                sc.ServiceName = WebDAVServiceName;
                sc.Stop();
            }
            catch
            {
            }
        }

        protected override void OnShutdown()
        {
            if (Server != null)
            {
                Server.Stop();
            }

            Server = null;
            base.OnShutdown();
        }

        protected override void OnStart(string[] args)
        {
            Server = new WebDAVServer(args);
            Server.Start();

            base.OnStart(args);
        }

        protected override void OnStop()
        {
            if (Server != null)
            {
                Server.Stop();
            }

            Server = null;
            base.OnStop();
        }

    }
}
