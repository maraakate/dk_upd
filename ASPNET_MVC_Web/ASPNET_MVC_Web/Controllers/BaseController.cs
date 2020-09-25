using System.Collections.Generic;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
   public abstract partial class BaseController : Controller
   {
      public static readonly string SQLConnStr = "Server=maraakate.org;Database=Daikatana;uid=dkro;pwd=dkro;timeout=600;"; /* FS: FIXME: Move this to web.config. */
      public const int BUILD = 0;
      public const int DEBUGSYMBOL = 1;
      public const int PAK = 2;

      public const int ARCHWIN32 = 0;
      public const int ARCHWIN64 = 1;
      public const int ARCHLINUX32 = 2;
      public const int ARCHLINUX64 = 3;
      public const int ARCHFREEBSD = 4;
      public const int ARCHOSX = 5;
      public static readonly List<string> ListArch = new List<string> { "Win32", "Win64", "Linux", "Linux_x64", "FreeBSD", "OSX" };
   }
}