/****** Script for SelectTopNRows command from SSMS  ******/
declare @guid uniqueidentifier;
set @guid =  NEWID();

SELECT @guid

insert into tblBuilds (id, date, arch, filename, changes)
values (@guid, '2020-09-04', 'Win64', 'DK_EXE_092420_SLIM_TEST_x64.EXE', '')

insert into tblDBSymbols (id, filename)
values(@guid, 'DK_PDB_092420_x64.7z')

insert into tblBuildsBinary(id, data)
values(@guid, (SELECT * FROM OPENROWSET(BULK N'C:\Apache\DK13\DK_EXE_092420_SLIM_TEST_x64.EXE', SINGLE_BLOB) AS Executable))

insert into tblDBSymbolsBinary(id, data)
values (@guid, (SELECT * FROM OPENROWSET(BULK N'C:\Apache\DK13\DK_PDB_092420_x64.7z', SINGLE_BLOB) AS Executable))

Update tblLatest
set id  = @guid
WHERE arch LIKE 'Win64%' AND beta = '1'
