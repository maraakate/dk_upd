using ASPNET_MVC_Web.Models;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Data.SqlClient;
using System.Linq;
using System.Text;
using System.Web;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
   public class ListController : Controller
   {
      static readonly string SQLConnStr = "Server=maraakate.org;Database=Daikatana;uid=dkro;pwd=dkro;timeout=600;"; /* FS: FIXME: Move this to web.config. */
      const int ALLBUILDS = 0;
      const int ALLBUILDSWITHSYMBOLS = 1;
      const int LATESTBUILDS = 2;

      const int ARCHWIN32 = 0;
      const int ARCHWIN64 = 1;
      const int ARCHLINUX32 = 2;
      const int ARCHLINUX64 = 3;
      const int ARCHFREEBSD = 4;
      const int ARCHOSX = 5;
      static readonly List<string> ListArch = new List<string> { "Win32", "Win64", "Linux", "Linux_x64", "FreeBSD", "OSX" };

      private bool GetList (ref ListViewModel model, int? type, int? arch, bool beta)
      {
         clsSQL dbSQL, dbSQLPDB;
         Collection<SqlParameter> Parameters;
         StringBuilder Query;
         string searchParams;
         int _type;

         if (model == null)
         {
            return false;
         }

         _type = 0;

         if (type != null)
         {
            _type = (int)type;
         }

         try
         {
            dbSQL = new clsSQL(SQLConnStr);
            dbSQLPDB = new clsSQL(SQLConnStr);
            Query = new StringBuilder(4096);
            Parameters = new Collection<SqlParameter>();
            searchParams = string.Empty;

            switch (_type)
            {
               case ALLBUILDS:
                  model.ListType = eListType.Standard;
                  Query.AppendLine("SELECT * FROM [Daikatana].[dbo].[tblBuilds] O");
                  searchParams = GetArch(ref model, arch);
                  Query.AppendLine(searchParams);
                  Query.AppendLine("ORDER BY [O].[arch], [O].[date]");
                  break;
               case ALLBUILDSWITHSYMBOLS:
                  model.ListType = eListType.WithDebugSymbols;
                  Query.AppendLine("SELECT [O].[id], [O].[date], [O].[arch], [O].[filename], [O].[changes], [I].[filename] FROM [Daikatana].[dbo].[tblBuilds] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblDBSymbols] I on ([I].[id]=[O].[id])");
                  searchParams = GetArch(ref model, arch);
                  Query.AppendLine(searchParams);
                  Query.AppendLine("ORDER BY [O].[arch], [O].[date]");
                  break;
               case LATESTBUILDS:
                  model.ListType = eListType.WithBeta;
                  Query.AppendLine("SELECT [O].[id], [I].[date], [O].[arch], [I].[filename], [I].[changes], [O].[beta] FROM [Daikatana].[dbo].[tblLatest] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblBuilds] I on ([I].[id]=[O].[id])");
                  searchParams = GetArch(ref model, arch);
                  GetBeta(ref model, ref searchParams, beta);
                  Query.AppendLine(searchParams);
                  Query.AppendLine("ORDER BY [O].[arch], [I].[date]");
                  break;
               default:
                  return false;
            }

            if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
            {
               model.Message = dbSQL.LastErrorMessage;
               return false;
            }

            while (dbSQL.Read())
            {
               Guid _id;
               string _arch;
               bool _beta;
               string filename_build;
               string filename_pdb;
               string _date;
               string _changes;
               string _url;
               string _urlPDB;

               _id = new Guid();
               _arch = string.Empty;
               filename_build = string.Empty;
               filename_pdb = string.Empty;
               _date = string.Empty;
               _changes = string.Empty;
               _beta = false;
               _url = string.Empty;
               _urlPDB = string.Empty;

               switch (_type)
               {
                  case ALLBUILDS:
                     _id = dbSQL.ReadGuid(0);
                     _date = dbSQL.ReadDateTime(1).ToShortDateString();
                     _arch = dbSQL.ReadString(2);
                     filename_build = dbSQL.ReadString(3);
                     _changes = dbSQL.ReadString(4);

                     _url = string.Format("../Download?id={0}&Type=0", _id.ToString());

                     filename_pdb = GetPDB(ref dbSQLPDB, _id);
                     if (string.IsNullOrWhiteSpace(filename_pdb) == false)
                     {
                        _urlPDB = string.Format("../Download?id={0}&Type=1", _id.ToString());
                     }

                     model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, fileNamePDB = filename_pdb, changes = _changes, url = _url, urlPDB = _urlPDB });
                     break;
                  case ALLBUILDSWITHSYMBOLS:
                     break;
                  case LATESTBUILDS:
                     _id = dbSQL.ReadGuid(0);
                     _date = dbSQL.ReadDateTime(1).ToShortDateString();
                     _arch = dbSQL.ReadString(2);
                     filename_build = dbSQL.ReadString(3);
                     _changes = dbSQL.ReadString(4);
                     _beta = dbSQL.ReadBool(5);

                     _url = string.Format("../Download?id={0}&Type=0", _id.ToString());

                     filename_pdb = GetPDB(ref dbSQLPDB, _id);
                     if (string.IsNullOrWhiteSpace(filename_pdb) == false)
                     {
                        _urlPDB = string.Format("../Download?id={0}&Type=1", _id.ToString());
                     }

                     model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, fileNamePDB = filename_pdb , changes = _changes, beta = _beta, url = _url, urlPDB = _urlPDB });
                     break;
                  default:
                     return false;
               }
            }
         }
         catch (Exception Ex)
         {
            model.Message = Ex.Message;
            return false;
         }

         return false;
      }

      private string GetArch (ref ListViewModel model, int? arch)
      {
         if (model == null)
         {
            return string.Empty;
         }

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
                  return String.Format("WHERE [O].[arch]='{0}'", ListArch[(int)arch]);
               default:
                  model.Message = "Invalid parameters for 'arch'.  Valid options are 0-5";
                  return string.Empty;
            }
         }

         return string.Empty;
      }

      private void GetBeta (ref ListViewModel model, ref string searchParams, bool beta)
      {
         if (model == null)
         {
            return;
         }

         if (searchParams == null)
         {
            model.Message = "searchParams is NULL!";
            return;
         }

         if (beta == false)
         {
            return;
         }

         if (searchParams.Length > 0)
         {
            searchParams += " AND [O].[beta]=1";
         }
         else
         {
            searchParams = "WHERE [O].[beta]=1";
         }
      }

      private string GetPDB (ref clsSQL dbSQL, Guid id)
      {
         string filename_pdb;
         StringBuilder Query;
         Collection<SqlParameter> Parameters;

         try
         {
            filename_pdb = string.Empty;
            Query = new StringBuilder(4096);
            Parameters = new Collection<SqlParameter>();

            Query.AppendLine("SELECT [I].[filename] FROM [Daikatana].[dbo].[tblBuilds] O");
            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblDBSymbols] I on ([I].[id]=[O].[id])");
            Query.AppendLine("WHERE [O].[id]=@id");

            Parameters.Add(clsSQL.BuildSqlParameter("@id", System.Data.SqlDbType.UniqueIdentifier, id));
            if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
            {
               return string.Empty;
            }

            if (dbSQL.Read())
            {
               filename_pdb = dbSQL.ReadString(0);
               return filename_pdb;
            }
         }
         catch (Exception ex)
         {
            return string.Empty;
         }

         return string.Empty;
      }

      public ActionResult Index (int? type, int? arch, int? beta)
      {
         ListViewModel model;

         model = new ListViewModel();

         if (beta != null && beta > 0)
         {
            GetList(ref model, LATESTBUILDS, arch, true);
         }
         else
         {
            GetList(ref model, type, arch, false);
         }

         return View(model);
      }
   }
}
