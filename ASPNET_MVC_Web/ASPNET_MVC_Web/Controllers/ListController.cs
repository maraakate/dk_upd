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

      private bool GetList (ref ListViewModel model, int? type)
      {
         clsSQL dbSQL;
         Collection<SqlParameter> Parameters;
         StringBuilder Query;
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
            const string contentType = @"application/octet-stream";

            dbSQL = new clsSQL(SQLConnStr);
            Query = new StringBuilder(4096);
            Parameters = new Collection<SqlParameter>();

            switch (_type)
            {
               case ALLBUILDS:
                  model.ListType = eListType.Standard;
                  Query.AppendLine("SELECT * FROM [Daikatana].[dbo].[tblBuilds]");
                  Query.AppendLine("ORDER BY [arch], [date]");
                  break;
               case ALLBUILDSWITHSYMBOLS:
                  model.ListType = eListType.WithDebugSymbols;
                  Query.AppendLine("SELECT [O].[id], [O].[date], [O].[arch], [O].[filename], [O].[changes], [I].[filename] FROM [Daikatana].[dbo].[tblBuilds] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblDBSymbols] I on ([I].[id]=[O].[id])");
                  Query.AppendLine("ORDER BY [O].[arch], [O].[date]");
                  break;
               case LATESTBUILDS:
                  model.ListType = eListType.WithBeta;
                  Query.AppendLine("SELECT [O].[id], [I].[date], [O].[arch], [I].[filename], [I].[changes], [O].[beta] FROM [Daikatana].[dbo].[tblLatest] O");
                  Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblBuilds] I on ([I].[id]=[O].[id])");
                  Query.AppendLine("ORDER BY [O].[arch], [I].[date]");
                  break;
               default:
                  return false;
            }

            if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
            {
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

               _id = new Guid();
               _arch = string.Empty;
               filename_build = string.Empty;
               filename_pdb = string.Empty;
               _date = string.Empty;
               _changes = string.Empty;
               _beta = false;
               _url = string.Empty;

               switch (_type)
               {
                  case ALLBUILDS:
                     _id = dbSQL.ReadGuid(0);
                     _date = dbSQL.ReadDateTime(1).ToShortDateString();
                     _arch = dbSQL.ReadString(2);
                     filename_build = dbSQL.ReadString(3);
                     _changes = dbSQL.ReadString(4);

                     _url = string.Format("../Download?id={0}&Type=0", _id.ToString());
                     model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, changes = _changes, url = _url });
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
                     model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, changes = _changes, beta = _beta, url = _url });
                     break;
                  default:
                     return false;
               }
            }
         }
         catch
         {
            return false;
         }

         return false;
      }

      public ActionResult Index (int? type)
      {
         ListViewModel model;

         model = new ListViewModel();

         GetList(ref model, type);

         return View(model);
      }
   }
}
