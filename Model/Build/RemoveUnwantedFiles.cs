//css_ref System;
//css_ref System.Core;
//css_ref System.Xml;
//css_include CSGeneral/Utility.cs
//css_include CSGeneral/StringManip.cs
//css_include CSGeneral/MathUtility.cs
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
//using CSGeneral;

class RemoveUnwantedFiles
{
    /// <summary>
    /// This program removes all SVN unversioned files from a specified directory.
    /// Can optionally do this recursively.
    /// </summary>
    static int Main(string[] args)
    {
        string[] ExtensionToDelete = new string[] {".map", ".obj", ".o", ".lib", ".mod", ".bak",
                                                   ".dsk", ".res", ".pch", ".dll", ".exe", ".so",
                                                   ".tds", ".a", ".pdb", ".imp", ".map", ".r"};

        Dictionary<string, string> Macros = CSGeneral.Utility.ParseCommandLine(args);
        try
        {
            // Make sure we have the right arguments.
            if (!Macros.ContainsKey("Directory"))
                throw new Exception("Usage: RemoveUnversionedFiles Directory=xxx [Recursive=Yes]");
            string directory = Macros["Directory"];
            string svnexe = "git.exe";
            if (Path.DirectorySeparatorChar == '/') svnexe = "git";
            string SVNFileName = CSGeneral.Utility.FindFileOnPath(svnexe);
            if (SVNFileName == "")
            { Console.WriteLine("Cannot find " + svnexe + " on PATH"); return (0); }

            // Start an SVN process to return a list of unversioned files.
            string Arguments = "status --porcelain";
            bool FullClean = false;
            if (Macros.ContainsKey("FullClean") && Macros["FullClean"] == "Yes")
                FullClean = true;

            Process P = CSGeneral.Utility.RunProcess(SVNFileName, Arguments, directory);
            string StdOut = CSGeneral.Utility.CheckProcessExitedProperly(P);
            string[] StdOutLines = StdOut.Split("\r\n".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);

            // Loop through all lines the SVN process produced.
            foreach (string line in StdOutLines)
            {
                if (line.Length > 3)
                {
                    string relativePath = line.Substring(3);
                    string path = Path.Combine(directory, relativePath);

                    bool DoDelete = false;
                    if (FullClean)
                        DoDelete = line[0] == '?' || line[0] == 'I';
                    else
                    {
                        bool ExtensionFound = Array.IndexOf(ExtensionToDelete, Path.GetExtension(path)) != -1;
                        DoDelete = (line[0] == '?' || line[0] == 'I') && ExtensionFound;
                    }

                    if (DoDelete)
                    {
                        
                        if (Directory.Exists(path))
                            Directory.Delete(path, true);
                        else if (File.Exists(path) && 
                                 !path.Contains("VersionStamper.exe") &&
                                 !path.Contains("JobScheduler.exe") &&
                                 !path.Contains("JobRunner.exe") &&
                                 !path.Contains("CSGeneral.dll"))
                        {
                            try
                            {Console.WriteLine("Deleting " + path);
                                File.Delete(path);
                            }
                            catch (Exception)
                            {
                                // Must be a locked or readonly file - ignore.
                            }
                        } else if (!File.Exists(path)) 
                        {
                            Console.WriteLine("Oops! \"" + path + "\" doesnt exist!");
                        }
                    }
                }
            }

            if (!FullClean)
            {
                string TclLink = Path.Combine(directory, "TclLink");
                try
                {
                    Directory.Delete(Path.Combine(TclLink, "bin"), true);
                    Directory.Delete(Path.Combine(TclLink, "doc"), true);
                    Directory.Delete(Path.Combine(TclLink, "include"), true);
                    Directory.Delete(Path.Combine(TclLink, "lib"), true);
                    File.Delete(Path.Combine(TclLink, "CIDataTypes.tcl"));
                }
                catch (Exception)
                { }
            }

        }
        catch (Exception err)
        {
            Console.WriteLine(err.Message);
            return 1;
        }
        Console.WriteLine("No errors!");

        return 0;
    }
}

