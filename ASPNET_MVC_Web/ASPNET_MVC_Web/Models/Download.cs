using System;

namespace ASPNET_MVC_Web.Models
{
   public class DownloadViewModel
   {
      public string Message { get; set; }

      public DownloadViewModel()
      {
         this.Clear();
      }

      private void Clear ()
      {
         Message = string.Empty;
      }
   }

   public class clsLatestBuilds
   {
      public Guid id { get; set; }
      public bool beta { get; set; }

      public clsLatestBuilds ()
      {
         this.Clear();
      }

      private void Clear()
      {
         id = new Guid();
         beta = false;
      }
   }
}
