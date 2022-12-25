using ASPNET_MVC_Web.Models;
using System;
using System.Collections.ObjectModel;
using System.Data.SqlClient;
using System.Linq;
using System.Text;
using System.Web.Mvc;

namespace ASPNET_MVC_Web.Controllers
{
    public class ListController : BaseController
    {
        const int ALLBUILDS = 0;
        const int ALLBUILDSWITHSYMBOLS = 1;
        const int LATESTBUILDS = 2;
        const int ALLPAKS = 3;
        const int LATESTPAKS = 4;

        private bool GetList (ref ListViewModel model, int? type, int? arch, bool beta)
        {
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
                using (clsSQL dbSQL = new clsSQL(SQLConnStr))
                {
                    clsSQL dbSQLPDB = new clsSQL(SQLConnStr);
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
                        case ALLPAKS:
                            model.ListType = eListType.PAKFiles;
                            Query.AppendLine("SELECT * FROM [Daikatana].[dbo].[tblPAKs] O");
                            Query.AppendLine("ORDER BY [O].[id], [I].[date]");
                            break;
                        case LATESTPAKS:
                            model.ListType = eListType.PAKFiles;
                            Query.AppendLine("SELECT [O].[id], [I].[date], [O].[type], [I].[filename] FROM [Daikatana].[dbo].[tblPAKsLatest] AS O");
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKs] I on ([i].[id]=[O].[id])");
                            Query.AppendLine("ORDER BY [O].[id], [I].[date]");
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

                        _id = new Guid();
                        _arch = string.Empty;
                        filename_build = string.Empty;
                        filename_pdb = string.Empty;
                        _date = string.Empty;
                        _changes = string.Empty;
                        _beta = false;

                        switch (_type)
                        {
                            case ALLBUILDS:
                            case ALLBUILDSWITHSYMBOLS: /* FS: Intentional fall through. */
                                _id = dbSQL.ReadGuid(0);
                                _date = dbSQL.ReadDateTime(1).ToShortDateString();
                                _arch = dbSQL.ReadString(2);
                                filename_build = dbSQL.ReadString(3);
                                _changes = dbSQL.ReadString(4);
                                filename_pdb = GetPDB(ref dbSQLPDB, _id);

                                model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, fileNamePDB = filename_pdb, changes = _changes });
                                break;
                            case LATESTBUILDS:
                                _id = dbSQL.ReadGuid(0);
                                _date = dbSQL.ReadDateTime(1).ToShortDateString();
                                _arch = dbSQL.ReadString(2);
                                filename_build = dbSQL.ReadString(3);
                                _changes = dbSQL.ReadString(4);
                                _beta = dbSQL.ReadBool(5);
                                filename_pdb = GetPDB(ref dbSQLPDB, _id);

                                model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, fileNamePDB = filename_pdb, changes = _changes, beta = _beta });
                                break;
                            case ALLPAKS:
                            case LATESTPAKS: /* FS: Intentional fall through. */
                                _id = dbSQL.ReadGuid(0);
                                _date = dbSQL.ReadDateTime(1).ToShortDateString();
                                _arch = dbSQL.ReadString(2);
                                filename_build = dbSQL.ReadString(3);
                                _changes = dbSQL.ReadString(4);
                                model.BinaryList.Add(new clsBinary { id = _id, date = _date, arch = _arch, fileName = filename_build, changes = _changes });
                                break;
                            default:
                                return false;
                        }
                    }
                }
            }
            catch (Exception Ex)
            {
                model.Message = Ex.Message;
                WriteLog("GetList(): Bad request from {0}.  searchParams is null.", Request.UserHostAddress);
                return false;
            }

            return true;
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
                    case ARCHDOS:
                        return String.Format("WHERE [O].[arch]='{0}'", ListArch[(int)arch]);
                    default:
                        model.Message = String.Format("Invalid parameters for 'arch'.  Valid options are 0-{0}", ListArch.Count-1);
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
                WriteLog("GetBeta(): Bad request from {0}.  searchParams is null.", Request.UserHostAddress);
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
                WriteLog("GetPDB(): Bad request from {0} {1}.  Reason: {2}", Request.UserHostAddress, id.ToString(), ex.Message);
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
