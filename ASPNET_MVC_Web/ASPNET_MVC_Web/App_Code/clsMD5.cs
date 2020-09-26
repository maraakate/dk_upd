using System;

public class clsMD5
{
   /* FS: From: https://stackoverflow.com/questions/10520048/calculate-md5-checksum-for-a-file */
   public static string CreateMD5(byte[] input)
   {
      byte[] hashBytes = { 0 };
      string hashStr = string.Empty;

      using (System.Security.Cryptography.MD5 md5 = System.Security.Cryptography.MD5.Create())
      {
         try
         {
            hashBytes = md5.ComputeHash(input);
            hashStr = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();
         }
         catch
         {

         }

         return hashStr;
      }
   }
}
