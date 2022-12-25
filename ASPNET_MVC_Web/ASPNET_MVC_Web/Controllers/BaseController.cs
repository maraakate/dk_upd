using System;
using System.Collections.Generic;
using System.IO;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
    public abstract partial class BaseController : Controller
    {
        public static readonly string SQLConnStr = "Server=127.0.0.1;Database=Daikatana;uid=dkro;pwd=dkro;timeout=600;"; /* FS: FIXME: Move this to web.config. */
        public const int BUILD = 0;
        public const int DEBUGSYMBOL = 1;
        public const int PAK = 2;

        public const int ARCHWIN32 = 0;
        public const int ARCHWIN64 = 1;
        public const int ARCHLINUX32 = 2;
        public const int ARCHLINUX64 = 3;
        public const int ARCHFREEBSD = 4;
        public const int ARCHOSX = 5;
        public const int ARCHDOS = 6;

        public const int PAK4 = 0;
        public const int PAK5 = 1;
        public const int PAK6 = 2;

        public static readonly List<string> ListArch = new List<string> { "Win32", "Win64", "Linux", "Linux_x64", "FreeBSD", "OSX", "DOS" };
        private string logFile = string.Format("dk_upd_error_log_{0}_{1}_{2}.txt", DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day);

        private void AppendDateToLog (ref string msg)
        {
            if (msg != null)
            {
                msg = String.Format("{0}: {1}", DateTime.Now, msg);
            }
        }

        public void WriteLog (string msg)
        {
            AppendDateToLog(ref msg);

            try
            {
                using (StreamWriter sw = new StreamWriter(logFile, true))
                {
                    sw.WriteLine(msg);
                    sw.Flush();
                }
            }
            catch (Exception ex)
            {
                Console.Write("Bad things: {0}\n", ex.Message);
            }
        }

        public void WriteLog (string msg, params object[] args)
        {
            string tmp;

            tmp = String.Format(msg, args);

            WriteLog(tmp);
        }
    }
}
