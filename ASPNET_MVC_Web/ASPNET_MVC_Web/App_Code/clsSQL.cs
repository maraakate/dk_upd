using System;
using System.ComponentModel;
using System.Data.SqlClient;

public class clsSQL : IDisposable
{
   public event ErrorEventHandler OnError;
   public delegate void ErrorEventHandler(object sender, ErrorEventArgs e);
   public class ErrorEventArgs : EventArgs
   {
      private string _QueryString;
      [Description("The Query String ")]
      public string QueryString { get { return _QueryString; } }

      private string _ErrorMessage;
      [Description("The Error Message ")]
      public string ErrorMessage { get { return _ErrorMessage; } }

      public ErrorEventArgs(string currentErrorMessage, string queryString)
      {
         this._ErrorMessage = currentErrorMessage;
         this._QueryString = queryString;
      }
   }
   protected virtual void ErrorOccured(ErrorEventArgs e)
   {
      // If there are registered clients raise event
      OnError?.Invoke(this, e);
   }

   private string pLastErrorMessage;
   public string LastErrorMessage { get { return pLastErrorMessage; } }

   public string LastQueryString { get; set; }

   private static string pApplicationName = string.Empty;

   public static string ApplicationName
   {
      get
      {
         //            if (string.IsNullOrWhiteSpace(pApplicationName))
         //               pApplicationName = UTILS.GetFriendlyAppName();

         return pApplicationName;
      }
      set
      {
         pApplicationName = value;
      }
   }

   private static string pSessionID;
   public static string SessionID
   {
      get
      {
         if (string.IsNullOrWhiteSpace(pSessionID))
            pSessionID = Guid.NewGuid().ToString();

         return pSessionID;
      }
   }

   public static int Timeout
   {
      get
      {
         if (pTimeOut == 0)
            pTimeOut = 30;

         return pTimeOut;
      }
      set
      {
         pTimeOut = value;
      }
   }
   public bool HasRows
   {
      get
      {
         if (this.Reader == null)
            return false;

         return this.Reader.HasRows;
      }
   }
   public int FieldCount
   {
      get
      {
         if (this.Reader == null)
            return 0;

         return this.Reader.FieldCount;
      }
   }

   private static int pTimeOut;

   private string ConnectionString { get; set; }
   private SqlConnection DBConnection { get; set; }
   private SqlCommand DBCommand { get; set; }
   private SqlTransaction DBTransaction { get; set; }
   private SqlDataReader Reader { get; set; }
   private bool disposed;

   public clsSQL(string constr)
   {
      LastQueryString = string.Empty;
      ConnectionString = constr;
      disposed = false;
      pLastErrorMessage = string.Empty;
   }

   public void Dispose()
   {
      Dispose(true);
      GC.SuppressFinalize(this);
   }

   protected virtual void Dispose(bool disposing)
   {
      // Check to see if Dispose has already been called.
      if (!this.disposed)
      {
         // If disposing equals true, dispose all managed and unmanaged resources.
         if (disposing)
         {
            if (this.Reader != null)
            {
               if (!this.Reader.IsClosed)
                  this.Reader.Close();

               this.Reader.Dispose();
               this.Reader = null;
            }

            if (this.DBTransaction != null)
               this.DBTransaction.Dispose();

            if (this.DBCommand != null)
               this.DBCommand.Dispose();

            if (this.DBConnection != null)
            {
               if (this.DBConnection.State == System.Data.ConnectionState.Open)
                  this.DBConnection.Close();

               this.DBConnection.Dispose();
               this.DBConnection = null;
            }
         }
         disposed = true;
      }
   }

   public bool BeginTransaction(string transactionName)
   {
      try
      {
         this.DBConnection = new SqlConnection(this.ConnectionString);
         this.DBConnection.Open();

         this.DBCommand = this.DBConnection.CreateCommand();
         this.DBCommand.CommandTimeout = pTimeOut;

         this.DBTransaction = DBConnection.BeginTransaction(transactionName);

         this.DBCommand.Connection = this.DBConnection;
         this.DBCommand.Transaction = this.DBTransaction;

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("BeginTransaction()", ex);
         return false;
      }

      return true;
   }

   public bool AddToTransaction(string query, SqlParameter[] parameters)
   {
      try
      {
         LastQueryString = query;

         if (parameters != null && parameters.Length > 0)
            this.DBCommand.Parameters.AddRange(parameters);

         this.DBCommand.CommandText = query;

         this.DBCommand.ExecuteNonQuery();

         this.DBCommand.Parameters.Clear();
         this.DBCommand.CommandText = string.Empty;
      }
      catch (Exception ex)
      {
         SetLastError(ex, query, parameters);
         Log("AddToTransaction()", ex);
         try
         {
            this.DBTransaction.Rollback();
         }
         catch (Exception ex2)
         {
            Log("AddToTransaction() - Rollback", ex2);
         }

         return false;
      }

      return true;
   }

   public bool CommitTransaction()
   {
      try
      {
         this.DBTransaction.Commit();
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("CommitTransaction()", ex);
         try
         {
            this.DBTransaction.Rollback();
         }
         catch (Exception ex2)
         {
            Log("CommitTransaction() - Rollback", ex2);
         }
         return false;
      }

      return true;
   }

   public bool RollBackTransaction()
   {
      try
      {
         this.DBTransaction.Rollback();
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("CommitTransaction()", ex);
         return false;
      }

      return true;
   }

   public bool Query(string query, SqlParameter[] parameters)
   {
      LastQueryString = query;

      if (this.DBConnection == null)
         this.DBConnection = new SqlConnection(ConnectionString);

      try
      {
         if (this.DBConnection.State != System.Data.ConnectionState.Open)
         {
            this.DBConnection.Open();
            using (SqlCommand comm = new SqlCommand("SET ARITHABORT ON;", this.DBConnection))
               comm.ExecuteNonQuery();
         }
      }
      catch (Exception ex)
      {
         SetLastError(ex, query, parameters);
         Log("Query() - open", ex);
         return false;
      }

      using (SqlCommand command = new SqlCommand(query, this.DBConnection))
      {
         if (parameters != null && parameters.Length > 0)
            command.Parameters.AddRange(parameters);

         command.CommandTimeout = pTimeOut;

         try
         {
            if (this.Reader != null && !this.Reader.IsClosed)
            {
               this.Reader.Close();
               this.Reader.Dispose();
               this.Reader = null;
            }

            this.Reader = command.ExecuteReader(System.Data.CommandBehavior.CloseConnection | System.Data.CommandBehavior.SingleResult);
         }
         catch (Exception ex)
         {
            SetLastError(ex, query, parameters);
            Log("Query() - reader", ex);
            return false;
         }
      }

      return true;
   }

   public bool Update(string query, SqlParameter[] parameters, out int RowsAffected)
   {
      LastQueryString = query;

      RowsAffected = 0;

      if (!query.ToLowerInvariant().Contains("where"))
      {
         Log("Update()", null, "Cannot update a record without a Where clause");
         return false;
      }

      if (!query.ToLowerInvariant().StartsWith("update"))
      {
         Log("Update()", null, "Cannot update a record without an Update statement");
         return false;
      }

      using (SqlConnection dbConn = new SqlConnection(ConnectionString))
      {

         try
         {
            dbConn.Open();
            using (SqlCommand comm = new SqlCommand("SET ARITHABORT ON", dbConn))
               comm.ExecuteNonQuery();
         }
         catch (Exception ex)
         {
            SetLastError(ex, query, parameters);
            Log("Update() - Open", ex);
            return false;
         }

         using (SqlCommand command = new SqlCommand(query, dbConn))
         {
            if (parameters != null && parameters.Length > 0)
               command.Parameters.AddRange(parameters);

            command.CommandTimeout = pTimeOut;

            try
            {
               RowsAffected = command.ExecuteNonQuery();
            }
            catch (Exception ex)
            {
               SetLastError(ex, query, parameters);
               Log("Update() - ExecuteNonQuery", ex);
               return false;
            }
         }
      }
      return true;
   }

   public bool Insert(string query, SqlParameter[] parameters, out int RowsAffected)
   {
      LastQueryString = query;

      RowsAffected = 0;

      if (!query.ToLowerInvariant().StartsWith("insert"))
      {
         Log("Insert()", null, "Cannot Insert a record without an Insert statement");
         return false;
      }

      using (SqlConnection dbConn = new SqlConnection(ConnectionString))
      {
         try
         {
            dbConn.Open();
            using (SqlCommand comm = new SqlCommand("SET ARITHABORT ON", dbConn))
               comm.ExecuteNonQuery();
         }
         catch (Exception ex)
         {
            SetLastError(ex, query, parameters);
            Log("Insert() - Open", ex);
            return false;
         }

         using (SqlCommand command = new SqlCommand(query, dbConn))
         {
            if (parameters != null && parameters.Length > 0)
               command.Parameters.AddRange(parameters);

            command.CommandTimeout = pTimeOut;

            try
            {
               RowsAffected = command.ExecuteNonQuery();
            }
            catch (Exception ex)
            {
               SetLastError(ex, query, parameters);
               Log("Insert() - ExecuteNonQuery", ex);
               return false;
            }
         }
      }

      return true;
   }

   public bool ExecuteNonQuery(string query, SqlParameter[] parameters)
   {
      LastQueryString = query;

      using (SqlConnection dbConn = new SqlConnection(ConnectionString))
      {
         try
         {
            dbConn.Open();
            using (SqlCommand comm = new SqlCommand("SET ARITHABORT ON", dbConn))
               comm.ExecuteNonQuery();
         }
         catch (Exception ex)
         {
            SetLastError(ex, query, parameters);
            Log("ExecuteNonQuery() - Open", ex);
            return false;
         }

         using (SqlCommand command = new SqlCommand(query, dbConn))
         {
            if (parameters != null && parameters.Length > 0)
               command.Parameters.AddRange(parameters);

            command.CommandTimeout = pTimeOut;

            try
            {
               command.ExecuteNonQuery();
            }
            catch (Exception ex)
            {
               SetLastError(ex, query, parameters);
               Log("ExecuteNonQuery() - ExecuteNonQuery", ex);
               return false;
            }
         }
      }

      return true;
   }

   public bool ExecuteNonQuery_Persistent(string query, SqlParameter[] parameters)
   {
      LastQueryString = query;

      if (this.DBConnection == null)
         this.DBConnection = new SqlConnection(ConnectionString);

      try
      {
         if (this.DBConnection.State != System.Data.ConnectionState.Open)
         {
            this.DBConnection.Open();
            using (SqlCommand comm = new SqlCommand("SET ARITHABORT ON", this.DBConnection))
               comm.ExecuteNonQuery();
         }
      }
      catch (Exception ex)
      {
         SetLastError(ex, query, parameters);
         Log("ExecuteNonQuery2() - open", ex);
         return false;
      }

      try
      {
         using (SqlCommand command = new SqlCommand(query, this.DBConnection))
         {
            if (parameters != null && parameters.Length > 0)
               command.Parameters.AddRange(parameters);

            command.CommandTimeout = pTimeOut;

            try
            {
               command.ExecuteNonQuery();
            }
            catch (Exception ex)
            {
               SetLastError(ex, query, parameters);
               Log("ExecuteNonQuery2() - ExecuteNonQuery", ex);
               return false;
            }
         }
      }
      catch (Exception ex)
      {
         SetLastError(ex, query, parameters);
         Log("ExecuteNonQuery2() - open", ex);
         return false;
      }

      return true;
   }

   public bool IsDBNull(int idx)
   {
      return this.Reader.IsDBNull(idx);
   }

   public int GetIndexForFieldName(string fieldName)
   {
      if (this.Reader == null)
         return -1;

      for (int i = 0; i < this.Reader.FieldCount; i++)
      {
         if (this.Reader.GetName(i).Equals(fieldName, StringComparison.OrdinalIgnoreCase))
            return i;
      }

      return -1;
   }

   public bool Read()
   {
      if (this.Reader == null)
         return false;

      return this.Reader.Read();
   }

   public DateTime ReadDateTime(int idx)
   {
      DateTime rtn;

      rtn = new DateTime();

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? new DateTime() : this.Reader.GetDateTime(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadDateTime(idx)", ex);
      }

      return rtn;
   }

   public DateTime ReadDateTime(string fieldName)
   {
      int idx;
      DateTime rtn;

      rtn = new DateTime();

      try
      {
         idx = GetIndexForFieldName(fieldName);
         if (idx == -1)
            return rtn;

         rtn = ReadDateTime(idx);

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadDateTime(fieldname)", ex);
      }

      return rtn;
   }

   public bool ReadBool(int idx, bool defaultValue = false)
   {
      bool rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetBoolean(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadBool()", ex);
      }

      return rtn;
   }

   public byte ReadByte(int idx, byte defaultValue = 0x0)
   {
      byte rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetByte(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadByte()", ex);
      }

      return rtn;
   }

   public string ReadString(int idx, string defaultValue = "")
   {
      string rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetString(idx).Trim();
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadString()", ex);
      }

      return rtn;
   }

   public string ReadStringRaw(int idx, string defaultValue = "")
   {
      string rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetString(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadStringRaw()", ex);
      }

      return rtn;
   }

   public string ReadString(string fieldName, string defaultValue = "")
   {
      int idx;
      string rtn;

      rtn = defaultValue;

      try
      {
         idx = GetIndexForFieldName(fieldName);
         if (idx == -1)
            return rtn;

         rtn = ReadString(idx, defaultValue);

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadString(fieldname)", ex);
      }

      return rtn;
   }

   public string ReadStringRaw(string fieldName, string defaultValue = "")
   {
      int idx;
      string rtn;

      rtn = defaultValue;

      try
      {
         idx = GetIndexForFieldName(fieldName);
         if (idx == -1)
            return rtn;

         rtn = ReadStringRaw(idx, defaultValue);

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadStringRaw(fieldname)", ex);
      }

      return rtn;
   }

   public int ReadTinyInt(int idx, int defaultValue = 0)
   {
      int rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : (int)this.Reader.GetByte(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadInt()", ex);
      }

      return rtn;
   }

   public int ReadSmallInt(int idx, int defaultValue = 0)
   {
      int rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetInt16(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadInt()", ex);
      }

      return rtn;
   }

   public int ReadInt(int idx, int defaultValue = 0)
   {
      int rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetInt32(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadInt()", ex);
      }

      return rtn;
   }

   public int ReadInt(string fieldName, int defaultValue = 0)
   {
      int idx;
      int rtn;

      rtn = defaultValue;

      try
      {
         idx = GetIndexForFieldName(fieldName);
         if (idx == -1)
            return rtn;

         rtn = ReadInt(idx, defaultValue);

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadInt(fieldname)", ex);
      }

      return rtn;
   }

   public long ReadLong(int idx, long defaultValue = 0)
   {
      long rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : (long)this.Reader.GetValue(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadLong()", ex);
      }

      return rtn;
   }

   public float ReadFloat(int idx, float defaultValue = 0)
   {
      float rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : (float)this.Reader.GetValue(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadFloat()", ex);
      }

      return rtn;
   }

   public double ReadDouble(int idx, double defaultValue = 0)
   {
      double rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : (double)this.Reader.GetValue(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadDouble()", ex);
      }

      return rtn;
   }

   public decimal ReadDecimal(int idx, decimal defaultValue = 0)
   {
      decimal rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : (decimal)this.Reader.GetValue(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadDecimal()", ex);
      }

      return rtn;
   }

   public byte[] ReadByteBuffer(int idx, byte[] defaultValue = null)
   {
      byte[] rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? defaultValue : this.Reader.GetValue(idx) as byte[];

      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadByteBuffer", ex);
      }

      return rtn;
   }

   public byte[] ReadVarBinary(int idx, byte[] defaultValue = null)
   {
      byte[] rtn;

      rtn = defaultValue;

      try
      {
         if (!this.Reader.IsDBNull(idx))
         {
            long size = this.Reader.GetBytes(idx, 0, null, 0, 0); //get the length of data 
            rtn = new byte[size];
            int bufferSize = 1024;
            long bytesRead = 0;
            int curPos = 0;
            while (bytesRead < size)
            {
               bytesRead += this.Reader.GetBytes(idx, curPos, rtn, curPos, bufferSize);
               curPos += bufferSize;
            }
         }

         return rtn;
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadByteBuffer", ex);
      }

      return rtn;
   }

   public Guid ReadGuid(int idx)
   {
      Guid rtn;

      rtn = Guid.Empty;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.IsDBNull(idx) ? Guid.Empty : this.Reader.GetGuid(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadGuid", ex);
      }

      return rtn;
   }

   public object ReadValue(int idx, object defaultValue = null)
   {
      object rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.GetValue(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadFloat()", ex);
      }

      return rtn;
   }

   public string GetFieldName(int idx, string defaultValue = "")
   {
      string rtn;

      rtn = defaultValue;

      try
      {
         if (this.Reader == null)
            return rtn;

         if (idx < 0 || idx > this.Reader.FieldCount - 1)
            return rtn;

         rtn = this.Reader.GetName(idx);
      }
      catch (Exception ex)
      {
         SetLastError(ex, string.Empty, null);
         Log("ReadFloat()", ex);
      }

      return rtn;
   }

   private void SetLastError(Exception ex, string queryString, SqlParameter[] parameters)
   {
      string AllTheErrorMessages;

      AllTheErrorMessages = string.Empty;

        //         UTILS.GetAllExceptions(ex, ref AllTheErrorMessages);

        pLastErrorMessage += ex.Message;

      ErrorOccured(new ErrorEventArgs(AllTheErrorMessages, DeParameterize(queryString, parameters)));
   }

   public static SqlParameter BuildSqlParameter(string parameterName, System.Data.SqlDbType which, object value)
   {
      SqlParameter rtn;

      rtn = new SqlParameter(parameterName, which);
      if (value == null)
         rtn.Value = DBNull.Value;
      else
         rtn.Value = value;

      return rtn;
   }

   public static string BuildWildcardString(string searchTerm)
   {
      string rtn;

      if (string.IsNullOrWhiteSpace(searchTerm))
         return string.Empty;

      rtn = searchTerm.Replace("[", "[]]");
      rtn = rtn.Replace("_", "[_]");
      rtn = rtn.Replace("*", "%");
      rtn = rtn.Replace("?", "_");

      rtn = MakeSQLSafe(rtn);

      return rtn;
   }

   public static string MakeSQLSafe(string str)
   {
      if (str == null)
         return string.Empty;

      return str.Replace("'", "''");
   }

   public static string DeParameterize(string query, SqlParameter[] parameters)
   {
      string rtn;

      if (string.IsNullOrWhiteSpace(query))
         return string.Empty;

      rtn = query;

      if (parameters == null || parameters.Length == 0)
         return rtn;

      foreach (SqlParameter p in parameters)
      {
         if (p.SqlDbType == System.Data.SqlDbType.Char || p.SqlDbType == System.Data.SqlDbType.Date
            || p.SqlDbType == System.Data.SqlDbType.DateTime || p.SqlDbType == System.Data.SqlDbType.DateTime2
            || p.SqlDbType == System.Data.SqlDbType.NChar || p.SqlDbType == System.Data.SqlDbType.NText
            || p.SqlDbType == System.Data.SqlDbType.NVarChar || p.SqlDbType == System.Data.SqlDbType.Text
            || p.SqlDbType == System.Data.SqlDbType.Time || p.SqlDbType == System.Data.SqlDbType.UniqueIdentifier
            || p.SqlDbType == System.Data.SqlDbType.VarChar)
            rtn = rtn.Replace(p.ParameterName, string.Format("'{0}'", MakeSQLSafe(p.Value.ToString())));
         else
            rtn = rtn.Replace(p.ParameterName, p.Value.ToString());
      }

      return rtn;
   }

   private void Log(string functionName, Exception ex, string extraMessage = "")
   {
      //         clsLogging Logging;

      //         Logging = new clsLogging();
      //         Logging.WriteLog("clsSQL.cs", functionName, string.Format("{0}\r\n{1}", extraMessage, LastQueryString), ex);
   }
}
