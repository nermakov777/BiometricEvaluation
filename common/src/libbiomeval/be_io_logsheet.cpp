/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <iomanip>

#include <be_io_utility.h>
#include <be_io_logsheet.h>

using namespace BiometricEvaluation;

const string BiometricEvaluation::IO::LogSheet::DescriptionTag("Description:");

static inline bool
lineIsLogEntry(
    string line)
{
	return (line[0] == IO::LogSheet::EntryDelimiter &&
	     std::isdigit(line[1]));
}

static inline bool
lineIsComment(
    string line)
{
	return (line[0] == IO::LogSheet::CommentDelimiter);
}

inline void
BiometricEvaluation::IO::LogSheet::updateCursor()
    throw (Error::FileError)
{
	if ((_cursor = _sequenceFile->tellg()) == -1)
		throw Error::FileError("Updating sequence cursor");
}

inline string
BiometricEvaluation::IO::LogSheet::trim(
    const string &entry)
{
	/* Length of a LogSheet entry prefix */
	static const uint8_t entryDelimiterLength = 12;
	/* Length of a LogSheet prefix */
	static const uint8_t commentDelimiterLength = 2;
	
	if (entry.length() <= 0)
		return (entry);
		
	switch (entry[0]) {
	case CommentDelimiter:
		return (entry.substr(commentDelimiterLength));
	case EntryDelimiter:
		return (entry.substr(entryDelimiterLength));
	default:
		return (entry);
	}
}

BiometricEvaluation::IO::LogSheet::LogSheet(
    const string &name,
    const string &description,
    const string &parentDir)
    throw (Error::ObjectExists, Error::StrategyError) : 
    std::ostringstream(),
    _cursor(0)
{
	if (!IO::Utility::validateRootName(name))
		throw Error::StrategyError("Invalid LogSheet name");

	string pathname;
	if (parentDir.empty() || parentDir == ".")
                pathname = name;
        else
                pathname = parentDir + '/' + name;

	if (IO::Utility::fileExists(pathname))
		throw Error::ObjectExists();

	/* Open the log sheet file as a file output stream */
	std::fstream *ofs = new std::fstream(pathname.c_str(), ios_base::out);
	if (!ofs)
		throw Error::StrategyError("Could not open LogSheet file");

	_theLogFile.reset(ofs);
	*_theLogFile << DescriptionTag << " " << description << endl;
	if (_theLogFile->fail())
		throw Error::StrategyError("Could not write description to "
		    "log file");
	_autoSync = false;
	_entryNumber = 1;
	
	_sequenceFile.reset(new fstream(pathname.c_str(), in));
	if (_sequenceFile->fail())
		throw Error::StrategyError("Could not open LogSheet sequencer");
}

BiometricEvaluation::IO::LogSheet::LogSheet(
    const string &name,
    const string &parentDir)
    throw (Error::ObjectDoesNotExist, Error::StrategyError) :
    std::ostringstream(),
    _cursor(0)
{
	if (!IO::Utility::validateRootName(name))
		throw Error::StrategyError("Invalid LogSheet name");

	string pathname;
	if (parentDir.empty() || parentDir == ".")
                pathname = name;
        else
                pathname = parentDir + '/' + name;

	if (!IO::Utility::fileExists(pathname))
		throw Error::ObjectDoesNotExist();

	/* Open the log sheet file as a file input stream so we can
	 * obtain the last entry number. */
	std::ifstream ifs(pathname.c_str(), in);
	if (ifs.fail())
		throw Error::StrategyError("Could not open LogSheet file");

	/*
	 * Determine the current entry number by counting lines that
	 * begin with the entry delimiter, etc.
	 */
	_entryNumber = 1;
	string oneline;
	while (!ifs.eof()) {
		std::getline(ifs, oneline);
		if (lineIsLogEntry(oneline))
			_entryNumber++;
	}

	/* Open the log sheet file as a file output stream */
	std::fstream *fs = new std::fstream(pathname.c_str(), app | out);
	if (!fs)
		throw Error::StrategyError("Could not open LogSheet file");

	_theLogFile.reset(fs);
	_autoSync = false;
	
	_sequenceFile.reset(new fstream(pathname.c_str(), in));
	if (_sequenceFile->fail())
		throw Error::StrategyError("Could not open LogSheet sequencer");
}

void
BiometricEvaluation::IO::LogSheet::write(const string &entry)
    throw (Error::StrategyError)
{
	*_theLogFile << EntryDelimiter << setw(10) << setfill('0') <<
	    _entryNumber << ' ' << entry << endl;
	if (_theLogFile->fail()) {
		ostringstream sbuf;
		sbuf << "Failed writing entry " << _entryNumber << 
		    " to log file";
		throw Error::StrategyError(sbuf.str());
	}
	if (_autoSync)
		this->sync();
	_entryNumber++;
}

void
BiometricEvaluation::IO::LogSheet::writeComment(
    const string &comment)
    throw (Error::StrategyError)
{
	*_theLogFile << CommentDelimiter << ' ' << comment << endl;
	if (_theLogFile->fail())
		throw Error::StrategyError();
	if (_autoSync)
		this->sync();
}

string
BiometricEvaluation::IO::LogSheet::getCurrentEntry()
{
	return (this->str());
}

uint32_t
BiometricEvaluation::IO::LogSheet::getCurrentEntryNumber()
{
	return (_entryNumber);
}

void
BiometricEvaluation::IO::LogSheet::resetCurrentEntry()
{
	this->seekp(beg);
	this->str("");
}

void
BiometricEvaluation::IO::LogSheet::newEntry()
    throw (Error::StrategyError)
{
	try {
		this->write(this->str());
	} catch (Error::StrategyError &e) {
		throw e;
	}
	this->resetCurrentEntry();
}

void
BiometricEvaluation::IO::LogSheet::sync()
    throw (Error::StrategyError)
{
	_theLogFile->flush();
	if (_theLogFile->fail())
		throw Error::StrategyError("Could not sync the log file");
}

void
BiometricEvaluation::IO::LogSheet::setAutoSync(
    bool state)
{
	_autoSync = state;	
}

string
BiometricEvaluation::IO::LogSheet::sequence(
    bool comments,
    bool trim,
    int32_t cursor)
    throw (Error::FileError,
    Error::ObjectDoesNotExist,
    Error::StrategyError)
{	
	if ((cursor != BE_LOGSHEET_SEQ_START) &&
	    (cursor != BE_LOGSHEET_SEQ_NEXT))
		throw Error::StrategyError("Invalid cursor position as " 
		    "argument");
	
	/* Sync to make sure that fstream knows about recent writes */
	_sequenceFile->sync();
	/* Reset EOF, then perform a small read to check for new entries */
	_sequenceFile->clear();
	_sequenceFile->peek();
	
	/* Reset stream and cursor when starting over */
	if ((cursor == BE_LOGSHEET_SEQ_START) ||
	    ((_cursor == 0) && (cursor == BE_LOGSHEET_SEQ_NEXT))) {
		_sequenceFile->clear();
		_sequenceFile->seekg(0, ios::beg);
		this->updateCursor();
	} else
		_sequenceFile->seekg(_cursor, ios::beg);
		
	/* Check that new cursor position is not the EOF */
	if (_sequenceFile->eof())
		throw Error::ObjectDoesNotExist();
	
	/* Get next entry, skipping any comments or descriptions */
	string entry = "", line = "";
	while (
	    /* Skip description and comments */
	    (comments == false && lineIsLogEntry(entry) == false) ||
	    /* or just skip description */
	    (comments == true && lineIsLogEntry(entry) == false &&
	    lineIsComment(entry) == false)) {
		if (_sequenceFile->eof())
			throw Error::ObjectDoesNotExist();
		
		getline(*_sequenceFile, entry);
		if (_sequenceFile->fail())
			throw Error::StrategyError("Priming sequence "
			    "read failed");
	}
	
	/* Check for multiline entry */
	this->updateCursor();
	getline(*_sequenceFile, line);
	while ((_sequenceFile->fail() == false) &&
	    (lineIsLogEntry(line) == false) &&
	    (lineIsComment(line) == false)) {
		entry += '\n' + line;
		this->updateCursor();
		getline(*_sequenceFile, line);
	}
	if (_sequenceFile->eof())
		return (trim ? LogSheet::trim(entry) : entry);
	if (_sequenceFile->fail())
		throw Error::StrategyError("Failed sequencing multiline entry");
	
	return (trim ? LogSheet::trim(entry) : entry);
}


void
BiometricEvaluation::IO::LogSheet::mergeLogSheets(
    vector< tr1::shared_ptr<LogSheet> > &logSheets)
    throw (Error::FileError,
    Error::StrategyError)
{
	if (logSheets.size() == 1)
		return;
		
	tr1::shared_ptr<LogSheet> master;
	try {
		master = logSheets.at(0);
	} catch (out_of_range) {
		throw Error::StrategyError("out_of_range 0");
	}
	
	bool firstEntry = true;
	vector< tr1::shared_ptr<LogSheet> >::const_iterator it;
	string entry;
	for (it = logSheets.begin() + 1; it != logSheets.end(); it++) {
		for (;;) {
			try {
				if (firstEntry)
					entry = (*it)->sequence(true, false,
					    BE_LOGSHEET_SEQ_START);
				else
					entry = (*it)->sequence(true, false,
					    BE_LOGSHEET_SEQ_NEXT);
			} catch (Error::ObjectDoesNotExist) {
				break;
			}

			if (lineIsComment(entry))
				master->writeComment(LogSheet::trim(entry));
			else
				master->write(LogSheet::trim(entry));
				
			firstEntry = false;
		}
		
		master->sync();
	}
	
}

BiometricEvaluation::IO::LogSheet::~LogSheet()
{
	_theLogFile->close();
}
