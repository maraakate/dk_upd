using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace ASPNET_MVC_Web.Models
{
   public class HelpViewModel
   {
      public List<string> ListArch { get; set; }
      public HelpViewModel()
      {
         this.Clear();
      }

      private void Clear ()
      {
         ListArch = new List<string>();
      }
   }
}