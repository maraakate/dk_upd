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
    public class DownloadController : BaseController
    {
        static readonly string APIVERSION = "1";

        [HttpPost]
        public FileResult _GetAPIVersion()
        {
            string fileName = "version.txt";

            return File(Encoding.UTF8.GetBytes(APIVERSION), "text/plain", fileName);
        }

        [HttpPost]
        public FileResult _GetMD5(int? type, int? arch, int? beta, int? pak)
        {
            int _beta = 0;
            string fileName = "error.txt";
            StringBuilder Query = new StringBuilder(4096);
            Collection<SqlParameter> Parameters = new Collection<SqlParameter>();

            if (type == null)
            {
                WriteLog("GetMD5(): Bad request from {0}.  type is null.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((type == BUILD) && (arch == null))
            {
                WriteLog("GetMD5(): Bad request from {0}.  arch is null for BUILD query.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((type == PAK) && (pak == null))
            {
                WriteLog("GetMD5(): Bad request from {0}.  pak is null for PAK query.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((beta != null) && (beta > 0))
            {
                _beta = 1;
            }

            switch (type)
            {
                case BUILD:
                    Query.AppendLine("SELECT [O].[md5] FROM [Daikatana].[dbo].tblBuildsBinary AS O");

                    switch (arch)
                    {
                        case ARCHWIN32:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Win32')");
                            fileName = "dk_win32.md5";
                            break;
                        case ARCHWIN64:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Win64')");
                            fileName = "dk_win64.md5";
                            break;
                        case ARCHLINUX32:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Linux')");
                            fileName = "dk_linux.md5";
                            break;
                        case ARCHLINUX64:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Linux_x64')");
                            fileName = "dk_linux_x64.md5";
                            break;
                        case ARCHFREEBSD:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='FreeBSD')");
                            fileName = "dk_freebsd_x64.md5";
                            break;
                        case ARCHOSX:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='OSX')");
                            fileName = "dk_osx.md5";
                            break;
                        case ARCHDOS:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='DOS')");
                            fileName = "dk_dos.md5";
                            break;
                        default:
                            goto errorFile;
                    }
                    Parameters.Add(clsSQL.BuildSqlParameter("@beta", System.Data.SqlDbType.Bit, _beta));
                    break;
                case PAK:
                    Query.AppendLine("SELECT [O].[md5] FROM [Daikatana].[dbo].[tblPAKsBinary] AS O");
                    switch (pak)
                    {
                        case PAK4:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak4.pak')");
                            fileName = "pak4.md5";
                            break;
                        case PAK5:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak5.pak')");
                            fileName = "pak5.md5";
                            break;
                        case PAK6:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak6.pak')");
                            fileName = "pak6.md5";
                            break;
                        default:
                            goto errorFile;
                    }
                    break;
                default:
                    goto errorFile;
            }

            try
            {
                using (clsSQL dbSQL = new clsSQL(SQLConnStr))
                {
                    if (dbSQL.Query(Query.ToString(), Parameters.ToArray()) == false)
                    {
                        WriteLog("GetMD5(): Bad request from {0}.  Query failed.  Reason: {1}\n", Request.UserHostAddress, dbSQL.LastErrorMessage);
                        goto errorFile;
                    }

                    if (dbSQL.Read())
                    {
                        return File(Encoding.UTF8.GetBytes(dbSQL.ReadString(0)), "text/plain", fileName);
                    }
                }
            }
            catch (Exception ex)
            {
                WriteLog("GetMD5(): Bad request from {0}.  Query failed.  Reason: {1}\n", Request.UserHostAddress, ex.Message);
            }

        errorFile:
            Response.StatusCode = 404;
            Response.TrySkipIisCustomErrors = true;
            throw new HttpException(404, "Not found");
        }

        [HttpPost]
        public FileResult _GetFileName(int? type, int? arch, int? beta, int? pak)
        {
            int _beta = 0;
            string fileName = "error.txt";
            StringBuilder Query = new StringBuilder(4096);
            Collection<SqlParameter> Parameters = new Collection<SqlParameter>();

            if (type == null)
            {
                WriteLog("GetFileName(): Bad request from {0}.  type is null.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((type == BUILD) && (arch == null))
            {
                WriteLog("GetFileName(): Bad request from {0}.  build is null for BUILD query.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((type == PAK) && (pak == null))
            {
                WriteLog("GetFileName(): Bad request from {0}.  pak is null for PAK query.", Request.UserHostAddress);
                goto errorFile;
            }

            if ((beta != null) && (beta > 0))
            {
                _beta = 1;
            }

            switch (type)
            {
                case BUILD:
                    Query.AppendLine("SELECT [O].[filename] FROM [Daikatana].[dbo].[tblBuilds] AS O");

                    switch (arch)
                    {
                        case ARCHWIN32:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Win32')");
                            fileName = "dk_win32.txt";
                            break;
                        case ARCHWIN64:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Win64')");
                            fileName = "dk_win64.txt";
                            break;
                        case ARCHLINUX32:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Linux')");
                            fileName = "dk_linux.txt";
                            break;
                        case ARCHLINUX64:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='Linux_x64')");
                            fileName = "dk_linux_x64.txt";
                            break;
                        case ARCHFREEBSD:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='FreeBSD')");
                            fileName = "dk_freebsd_x64.txt";
                            break;
                        case ARCHOSX:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='OSX')");
                            fileName = "dk_osx.txt";
                            break;
                        case ARCHDOS:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblLatest] I ON ([I].[id]=[O].[id] AND [I].[beta]=@beta AND [I].[arch]='DOS')");
                            fileName = "dk_dos.txt";
                            break;
                        default:
                            goto errorFile;
                    }
                    Parameters.Add(clsSQL.BuildSqlParameter("@beta", System.Data.SqlDbType.Bit, _beta));
                    break;
                case PAK:
                    Query.AppendLine("SELECT [O].[filename] FROM [Daikatana].[dbo].[tblPAKs] AS O");

                    switch (pak)
                    {
                        case PAK4:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak4.pak')");
                            fileName = "pak4.txt";
                            break;
                        case PAK5:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak5.pak')");
                            fileName = "pak5.txt";
                            break;
                        case PAK6:
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsLatest] I ON ([I].[id]=[O].[id] AND [I].[type]='pak6.pak')");
                            fileName = "pak6.txt";
                            break;
                        default:
                            goto errorFile;
                    }
                    break;
                default:
                    goto errorFile;
            }

            try
            {
                using (clsSQL dbSQL = new clsSQL(SQLConnStr))
                {
                    if (dbSQL.Query(Query.ToString(), Parameters.ToArray()) == false)
                    {
                        WriteLog("GetFileName(): Bad request from {0}.  Query failed.  Reason: {1}\n", Request.UserHostAddress, dbSQL.LastErrorMessage);
                        goto errorFile;
                    }

                    if (dbSQL.Read())
                    {
                        return File(Encoding.UTF8.GetBytes(dbSQL.ReadString(0)), "text/plain", fileName);
                    }
                }
            }
            catch (Exception ex)
            {
                WriteLog("GetFileName(): Bad request from {0}.  Query failed.  Reason: {1}\n", Request.UserHostAddress, ex.Message);
            }

        errorFile:
            Response.StatusCode = 404;
            Response.TrySkipIisCustomErrors = true;
            throw new HttpException(404, "Not found");
        }

        [HttpPost]
        public FileResult DownloadData(string id, int? type)
        {
            Guid _id;
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

            using (clsSQL dbSQL = new clsSQL(SQLConnStr))
            {
                try
                {
                    byte[] data;
                    const string contentType = @"application/octet-stream";
                    string filename;

                    _id = new Guid(id);
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
                        case PAK:
                            Query.AppendLine("FROM [Daikatana].[dbo].[tblPAKs] O");
                            Query.AppendLine("INNER JOIN [Daikatana].[dbo].[tblPAKsBinary] I ON ([I].[id] = [O].id)");
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

                    if (dbSQL.Read())
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
                    case ARCHDOS:
                        return ListArch[(int)arch];
                    default:
                        return string.Empty;
                }
            }

            return string.Empty;
        }

        private bool QueryLatestBuild(int? type, int? arch, int? beta, int? pak, ref DownloadViewModel model, out string id)
        {
            Collection<SqlParameter> Parameters;
            StringBuilder Query;
            List<clsLatestBuilds> builds;
            bool bWantBeta;

            id = string.Empty;

            if (model == null)
            {
                return false;
            }

            using (clsSQL dbSQL = new clsSQL(SQLConnStr))
            {
                try
                {
                    Query = new StringBuilder(4096);
                    Parameters = new Collection<SqlParameter>();
                    builds = new List<clsLatestBuilds>();
                    bWantBeta = false;

                    if (type == 0)
                    {

                        if (arch == null)
                        {
                            return false;
                        }

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
                            model.Message = String.Format("QueryLatestBuild(): Query failed for {0} {1} {2}.  Reason: {3}\n", Request.UserHostAddress, arch, bWantBeta, dbSQL.LastErrorMessage);
                            return false;
                        }
                    }
                    else if (type == 1)
                    {
                        return false; /* FS: TODO: Not yet implemented. */
                    }
                    else if (type == 2)
                    {
                        string typeParam;

                        Query.AppendLine("SELECT  distinct id FROM [Daikatana].[dbo].[tblPAKsLatest]");
                        if (pak == null)
                        {
                            return false;
                        }

                        switch (pak)
                        {
                            case 0:
                                typeParam = "pak4.pak";
                                break;
                            case 1:
                                typeParam = "pak5.pak";
                                break;
                            case 2:
                                typeParam = "pak6.pak";
                                break;
                            default:
                                return false;
                        }

                        Query.AppendLine("WHERE type=@type");
                        Parameters.Add(clsSQL.BuildSqlParameter("@type", System.Data.SqlDbType.NVarChar, typeParam));

                        if (!dbSQL.Query(Query.ToString(), Parameters.ToArray()))
                        {
                            model.Message = String.Format("QueryLatestBuild(): Query failed for {0} {1} {2}.  Reason: {3}\n", Request.UserHostAddress, type, pak, dbSQL.LastErrorMessage);
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }

                    while (dbSQL.Read())
                    {
                        Guid _id;
                        bool _beta;

                        _id = dbSQL.ReadGuid(0);
                        if (type == 0)
                            _beta = dbSQL.ReadBool(1);
                        else
                            _beta = false;

                        builds.Add(new clsLatestBuilds { id = _id, beta = _beta });
                    }

                    if (builds.Count == 0)
                    {
                        WriteLog("QueryLatestBuild(): No builds returned for {0} {1} {2}.", Request.UserHostAddress, arch, bWantBeta);
                        return false;
                    }

                    if (builds.Count > 2) /* FS: Something is not right in the DB. */
                    {
                        WriteLog("QueryLatestBuild(): More than 2 returned for {0} {1} {2} {3}.", Request.UserHostAddress, arch, bWantBeta, builds.Count);
                        return false;
                    }

                    if (bWantBeta == false && builds.Count != 1)
                    {
                        WriteLog("QueryLatestBuild(): More than 1 build returned for {0} {1} {2} {3}.", Request.UserHostAddress, arch, bWantBeta, builds.Count);
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
                    WriteLog("QueryLatestBuild(): Query failed from {0} {1}.  Reason: {2}", Request.UserHostAddress, arch, ex.Message);
                    return false;
                }
            }

            WriteLog("QueryLatestBuild(): No builds returned for {0} {1} {2}.", Request.UserHostAddress, arch, bWantBeta);
            return false;
        }

        public ActionResult GetAPIVersion()
        {
            return _GetAPIVersion();
        }

        public ActionResult GetFileName(int? type, int? arch, int? beta, int? pak)
        {
            return _GetFileName(type, arch, beta, pak);
        }

        public ActionResult GetMD5(int? type, int? arch, int? beta, int? pak)
        {
            return _GetMD5(type, arch, beta, pak);
        }

        public ActionResult GetLatestBuild(int? type, int? arch, int? beta, int? pak)
        {
            DownloadViewModel model;
            string id;
            int _type = 0;

            model = new DownloadViewModel();
            id = string.Empty;

            if ((type != null) && (type > 0))
            {
                _type = type.Value;
            }

            if (QueryLatestBuild(_type, arch, beta, pak, ref model, out id))
            {
                return DownloadData(id, _type);
            }

            WriteLog("GetLatestBuild(): Bad request from {0}", Request.UserHostAddress);

            Response.StatusCode = 404;
            Response.TrySkipIisCustomErrors = true;
            throw new HttpException(404, "Not found");
        }

        public ActionResult Index(string _id, int? type)
        {
            //DownloadViewModel model;

            //model = new DownloadViewModel();

            if (type != null)
            {
                return DownloadData(_id, type);
            }

            WriteLog("DownloadIndex(): Bad request from {0}", Request.UserHostAddress);

            Response.StatusCode = 404;
            Response.TrySkipIisCustomErrors = true;
            throw new HttpException(404, "Not found");
        }
    }
}
