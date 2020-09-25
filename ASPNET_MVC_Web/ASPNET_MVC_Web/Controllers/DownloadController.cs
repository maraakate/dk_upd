using ASPNET_MVC_Web.Models;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Data.SqlClient;
using System.Linq;
using System.Text;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
   public class DownloadController : BaseController
   {
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

      private string GetArch(int? arch)
      {
         if (arch != null)
         {
            switch (arch)
            {
               case ARCHWIN32:
               case ARCHWIN64:
               case ARCHLINUX32:
               case ARCHLINUX64:
               case ARCHFREEBSD:
               case ARCHOSX:
                  return ListArch[(int)arch];
               default:
                  return string.Empty;
            }
         }

         return string.Empty;
      }

      private bool QueryLatestBuild (int? beta, int? arch, ref DownloadViewModel model, out string id)
      {
         clsSQL dbSQL;
         Collection<SqlParameter> Parameters;
         StringBuilder Query;
         List<clsLatestBuilds> builds;
         bool bWantBeta;

         id = string.Empty;

         if (model == null)
         {
            return false;
         }

         if (arch == null)
         {
            return false;
         }

         try
         {
            dbSQL = new clsSQL(SQLConnStr);
            Query = new StringBuilder(4096);
            Parameters = new Collection<SqlParameter>();
            builds = new List<clsLatestBuilds>();
            bWantBeta = false;

            Query.AppendLine("SELECT distinct id, beta FROM [Daikatana].[dbo].[tblLatest]");
            if (beta == null)
            {
               Query.AppendLine("WHERE Arch=@arch AND Beta=0");
            }
            else
            {
               if ((beta < 0) || (beta == 0))
               {
                  Query.AppendLine("WHERE Arch=@arch AND Beta=0");
               }
               else
               {
                  Query.AppendLine("WHERE Arch=@arch"); /* FS: Latest release may be newer than beta.  So compare. */
                  bWantBeta = true;
               }
            }

            Parameters.Add(clsSQL.BuildSqlParameter("@arch", System.Data.SqlDbType.NVarChar, GetArch(arch)));

            if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
            {
               model.Message = String.Format("Query failed.  Reason: {0}\n", dbSQL.LastErrorMessage);
               return false;
            }

            while (dbSQL.Read())
            {
               Guid _id;
               bool _beta;

               _id = dbSQL.ReadGuid(0);
               _beta = dbSQL.ReadBool(1);

               builds.Add(new clsLatestBuilds {id = _id, beta = _beta });
            }

            if (builds.Count == 0)
            {
               model.Message = "No builds returned.\n";
               return false;
            }

            if (builds.Count > 2) /* FS: Something is not right in the DB. */
            {
               model.Message = "More than 2 builds returned.\n";
               return false;
            }

            if (bWantBeta == false && builds.Count != 1)
            {
               model.Message = "More than 1 build returned.\n";
               return false;
            }

            if (bWantBeta)
            {
               if (builds[0].id == builds[1].id) /* FS: Latest release is new/same as beta.  So give them the latest release. */
               {
                  id = builds[0].id.ToString();
                  return true;
               }
               else
               {
                  foreach (clsLatestBuilds build in builds)
                  {
                     if (build.beta == true)
                     {
                        id = build.id.ToString();
                        return true;
                     }
                  }
               }
            }
            else
            {
               id = builds[0].id.ToString();
               return true;
            }
         }
         catch (Exception ex)
         {
            model.Message = String.Format("Query failed.  Reason: {0}\n", ex.Message);
            return false;
         }

         model.Message = "No builds returned.\n";
         return false;
      }

      public ActionResult GetLatestBuild (int? arch, int? beta)
      {
         DownloadViewModel model;
         string id;

         model = new DownloadViewModel();
         id = string.Empty;

         if (QueryLatestBuild(beta, arch, ref model, out id))
         {
            return DownloadData(id, 0);
         }

         return View(model);
      }

      public ActionResult Index(string _id, int? type)
      {
         DownloadViewModel model;

         model = new DownloadViewModel();

         if (type != null)
         {
            return DownloadData(_id, type);
         }

         return View(model);
      }
   }
}
