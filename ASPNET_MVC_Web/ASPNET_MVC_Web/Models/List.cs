using System;
using System.Collections.Generic;

namespace ASPNET_MVC_Web.Models
{
   public enum eListType
   {
      Standard = 0,
      WithBeta = 1,
      WithDebugSymbols = 1,
      PAKFiles = 2,
   }

   public class ListViewModel
   {
      public List<clsBinary> BinaryList { get; set; }
      public string Message { get; set; }
      public eListType ListType { get; set; }

      public ListViewModel ()
      {
         this.Clear();
      }

      private void Clear ()
      {
         BinaryList = new List<clsBinary>();
         Message = string.Empty;
         ListType = eListType.Standard;
      }
   }

   public class clsBinary
   {
      public Guid id { get; set; }
      public string fileName { get; set; }
      public string fileNamePDB { get; set; }
      public string date { get; set; }
      public string arch { get; set; }
      public string changes { get; set; }
      public bool beta { get; set; }

      public clsBinary ()
      {
         this.Clear();
      }

      private void Clear ()
      {
         id = new Guid();
         fileName = string.Empty;
         fileNamePDB = string.Empty;
         date = string.Empty;
         arch = string.Empty;
         changes = string.Empty;
         beta = false;
      }
   }
}
