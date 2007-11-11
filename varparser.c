/*
Copyright (C) 2004-2007 Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The author can be reached at cwieninger@gmx.de

The project's page is at http://winni.vdr-developer.org/epgsearch
*/

#include "varparser.h"
#include <vdr/plugin.h>
#include "log.h"

bool cVarParser::Parse(const string& input)
{
   return ParseAssign(input);
}

bool cVarParser::ParseAssign(const string& input)
{   
   int assignPos = input.find("=");
   if (assignPos >= 0)
   {
      string var(input.begin(), input.begin() + assignPos);
      if (ParseVar(var))
      {
         varName = Strip(var);
         string assign(input.begin() + assignPos + 1, input.end());
         return ParseExp(assign);
      }
   }
   LogFile.eSysLog("error parsing '%s'", input.c_str());  
   return false;
}

bool cVarParser::ParseExp(const string& input)
{
   // system call?
   int sysPos = input.find("system");
   if (sysPos == 0)
      return ParseShellCmd(input);
   // conditional expression?
   int varPos = Strip(input).find("%");
   int queryPos = input.find("?");
   int colonPos = input.find(":");
   if (varPos == 0 && queryPos >= 0 && colonPos > queryPos)
      return ParseCondExp(input);
   else
   {
      // composed expression
      compExpr = input;
      return true; 
   }
}

bool cVarParser::ParseShellCmd(const string& input)
{
   int cmdPos = input.find("(");
   int cmdArgsBegin = input.find(",");
   int cmdArgsEnd = input.rfind(")");
   if (cmdPos == -1 || cmdArgsEnd == -1) return false;
   string shellcmd(input.begin() + cmdPos + 1, input.begin() + (cmdArgsBegin >= 0?cmdArgsBegin:cmdArgsEnd));
   shellcmd = Strip(shellcmd);

   cmdArgs = "";
   if (cmdArgsBegin >= 0)
      cmdArgs = string(input.begin() + cmdArgsBegin + 1, input.begin() + cmdArgsEnd);

   string cmdVDR = "varcmd: " + shellcmd;
   cmd = new cCommand;
   if (!cmd->Parse(cmdVDR.c_str()))
   {
      LogFile.eSysLog("error parsing command: %s", input.c_str());
      delete cmd;
      cmd = NULL;
      return false;
   }
   return true;
}

bool cVarParser::ParseCondExp(const string& input)
{   
   int condEndPos = input.find("?");
   string cond(input.begin(), input.begin() + condEndPos);
   int condNeqPos = cond.find("!=");
   int condEqPos = cond.find("==");

   if (condEqPos == -1 && condNeqPos == -1)
   {
      cond += "!=";
      condNeqPos = cond.find("!=");
   }

   if (condEqPos >= 0 || condNeqPos >= 0)
   {
      if (!ParseEquality(cond))
      {
         LogFile.eSysLog("error parsing '%s'", input.c_str());  
         return false;
      }
      condOp = (condEqPos >= 0)?condEq:condNeq;
   }
   else
   {
      LogFile.eSysLog("error parsing '%s'", input.c_str());  
      return false;
   }

   string truefalse(input.begin() + condEndPos + 1, input.end());
   int elsePos = truefalse.find(":");
   if (elsePos >= 0)
   {
      string truePart(truefalse.begin(), truefalse.begin() + elsePos);
      string falsePart(truefalse.begin() + elsePos + 1, truefalse.end());
      if (ParseVar(truePart) && ParseVar(falsePart))
      {
         condvarTrue = Strip(truePart); 
         condvarFalse = Strip(falsePart);		
         return true;
      }
   }
   LogFile.eSysLog("error parsing '%s'", input.c_str());  
   condEqLeft = condEqRight = "";
   return false;
}

bool cVarParser::ParseEquality(const string& input)
{
   int condEqPos = input.find("==");
   int condNeqPos = input.find("!=");
   int condOpPos = -1;
   if (condEqPos >= 0) condOpPos = condEqPos;
   if (condNeqPos >= 0) condOpPos = condNeqPos;
   if (condOpPos == -1) return false;
   string left(input.begin(), input.begin() + condOpPos);
   string right(input.begin() + condOpPos + 2, input.end());
   if (ParseExp(left) && ParseExp(right))
   {
      condEqLeft = Strip(left);
      condEqRight = Strip(right);
      return true;
   }
   return false;
}

bool cVarParser::ParseVar(const string& input)
{   
   string str = Strip(input);
   if (str[0] == '%' && str[str.size()-1] == '%' && str.size() > 2)
      return true;
   return false;
}

string cVarParser::Strip(const string& input)
{
   string str = input;
   while(str[0] == ' ') str.replace(0,1, "");
   while(str[str.size()-1] == ' ') str.replace(str.size()-1,1, "");
   return str;
}

bool cVarParser::IsCondExpr()
{
   return (condEqLeft != "");
}

bool cVarParser::IsShellCmd()
{
   return (cmd != NULL);
}
