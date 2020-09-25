using ASPNET_MVC_Web.Models;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
   public class HelpController : BaseController
   {
      private void Init (ref HelpViewModel model)
      {
         if (model == null)
         {
            return;
         }

         model.ListArch = ListArch;
      }

      public ActionResult Index()
      {
         HelpViewModel model;

         model = new HelpViewModel();

         Init(ref model);

         return View(model);
      }
   }
}