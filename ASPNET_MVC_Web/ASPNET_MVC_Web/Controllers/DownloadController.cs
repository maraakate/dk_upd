using ASPNET_MVC_Web;
using ASPNET_MVC_Web.Models;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Web;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
   public class DownloadController : Controller
   {
      static readonly string SQLConnStr = "Server=maraakate.org;Database=Daikatana;uid=dkro;pwd=dkro;timeout=600;"; /* FS: FIXME: Move this to web.config. */
      const int BUILD = 0;
      const int DEBUGSYMBOL = 1;
      const int PAK = 2;

      [HttpPost]
      public FileResult DownloadData (string id, int? type)
      {
         Guid _id;
         clsSQL dbSQL;
         Collection<SqlParameter> Parameters;
         StringBuilder Query;

         if (string.IsNullOrWhiteSpace(id))
         {
            return null;
         }

         if (type == null)
         {
            return null;
         }

         try
         {
            byte[] data;
            const string contentType = @"application/octet-stream";
            string filename;

            _id = new Guid(id);
            dbSQL = new clsSQL(SQLConnStr);
            Query = new StringBuilder(4096);
            Parameters = new Collection<SqlParameter>();
            filename = string.Empty;

            Query.AppendLine("SELECT [O].[filename], [I].[data]");
            switch (type)
            {
               case BUILD:
                  Query.AppendLine("FROM [Daikatana].[dbo].[tblBuilds] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblBuildsBinary] I ON ([I].[id] = [O].id)");
                  break;
               case DEBUGSYMBOL:
                  Query.AppendLine("FROM [Daikatana].[dbo].[tblDBSymbols] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblDBSymbolsBinary] I ON ([I].[id] = [O].id)");
                  break;
               default:
                  return null;
            }
            Query.AppendLine("WHERE [O].[id] = @id");

            Parameters.Add(clsSQL.BuildSqlParameter("@id", System.Data.SqlDbType.UniqueIdentifier, _id));

            if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
            {
               return null;
            }

            while (dbSQL.Read())
            {
               filename = dbSQL.ReadString(0, "DK_UNK");
               data = dbSQL.ReadByteBuffer(1, null);

               return File(data, contentType, filename);
            }
         }
         catch
         {
            return null;
         }

         return null;
      }

      public ActionResult Index(string id, int? type)
      {
         DownloadViewModel model;

         model = new DownloadViewModel();

         if (type != null)
         {
            return DownloadData(id, type);
         }

         return View();
      }
   }
}
