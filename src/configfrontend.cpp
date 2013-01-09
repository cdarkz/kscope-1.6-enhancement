/***************************************************************************
 *
 * Copyright (C) 2005 Elad Lahav (elad_lahav@users.sourceforge.net)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

#include <kstandarddirs.h>
#include "configfrontend.h"

/**
 * Class constructor.
 * @param	bAutoDelete	True to destroy the object when the process ends,
 *						false otherwise
 */
ConfigFrontend::ConfigFrontend(bool bAutoDelete) : 
	Frontend(1, bAutoDelete)
{
}

/**
 * Class destructor.
 */
ConfigFrontend::~ConfigFrontend()
{
}

/**
 * Executes the script using the "sh" shell.
 * @param	sCscopePath		If given, overrides the automatic check for Cscope's
 *							path
 * @param	sCtagsPath		If given, overrides the automatic check for Ctags'
 *							path
 * @param	sDotPath		If given, overrides the automatic check for Dot's
 *							path
 * @param	bCscopeOptsOnly	Only verify cscope's path and options
 * @return	true if successful, false otherwise
 */
bool ConfigFrontend::run(const QString& sCscopePath, 
	const QString& sCtagsPath, const QString& sDotPath,
	bool bCscopeOptsOnly)
{
	QStringList slArgs;
	KStandardDirs sd;
	QString sScript;
	
	// Execute using the user's shell
	setUseShell(true);
	
	// Find the configuration script
	sScript = sd.findResource("data", "kscope/kscope_config");
	if (sScript.isEmpty())
		return false;
		
	// Set command line arguments
	slArgs.append("sh");
	slArgs.append(sScript);
	
	if (bCscopeOptsOnly)
		slArgs.append("-co");
		
	// Initialise environment
	setEnvironment("CSCOPE_PATH", sCscopePath);
	setEnvironment("CTAGS_PATH", sCtagsPath);
	setEnvironment("DOT_PATH", sDotPath);
	
	// Parser initialisation
	m_delim = Newline;
	m_nNextResult = CscopePath;
	
	if (!Frontend::run("sh", slArgs))
		return false;
		
	emit test(CscopePath);
	return true;
}

/**
 * Handles tokens generated by the script.
 * Each token represents a line in the script's output, and is the result of
 * a different test.
 * @param	sToken	The generated token
 */
Frontend::ParseResult ConfigFrontend::parseStdout(QString& sToken, 
	ParserDelim)
{
	uint nResult;
	
	// Store the type of test for which the given token in the result
	nResult = m_nNextResult;
	
	// Determine the next test
	switch (m_nNextResult) {
	case CscopePath:
		if (sToken == "ERROR")
			m_nNextResult = CtagsPath;
		else
			m_nNextResult = CscopeVersion;
		break;
		
	case CscopeVersion:
		if (sToken == "ERROR")
			m_nNextResult = CtagsPath;
		else
			m_nNextResult = CscopeVerbose;
		break;
		
	case CscopeVerbose:
		m_nNextResult = CscopeSlowPath;
		break;
		
	case CscopeSlowPath:
		m_nNextResult = CtagsPath;
		break;
		
	case CtagsPath:
		if (sToken == "ERROR")
			m_nNextResult = END;
		else
			m_nNextResult = CtagsExub;
		break;
	
	case CtagsExub:
		if (sToken == "ERROR")
			m_nNextResult = END;
		else
			m_nNextResult = DotPath;
		break;
		
	case DotPath:
		if (sToken == "ERROR")
			m_nNextResult = END;
		else
			m_nNextResult = DotPlain;
		break;
	
	case DotPlain:
		m_nNextResult = END;
		break;
		
	case END:
		return DiscardToken;
	}
	
	// Publish the result and the type of the next test
	emit result(nResult, sToken);
	emit test(m_nNextResult);
	
	return DiscardToken;
}

#include "configfrontend.moc"