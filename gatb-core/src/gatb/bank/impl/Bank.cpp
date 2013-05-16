/*****************************************************************************
 *   GATB : Genome Assembly Tool Box                                         *
 *   Copyright (c) 2013                                                      *
 *                                                                           *
 *   GATB is free software; you can redistribute it and/or modify it under   *
 *   the CECILL version 2 License, that is compatible with the GNU General   *
 *   Public License                                                          *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   CECILL version 2 License for more details.                              *
 *****************************************************************************/

#include <gatb/bank/impl/Bank.hpp>

#include <gatb/system/impl/System.hpp>
#include <gatb/tools/misc/api/StringsRepository.hpp>
#include <gatb/tools/designpattern/impl/IteratorHelpers.hpp>

#include <algorithm>
#include <string.h>
#include <errno.h>
#include <zlib.h> // Added by Pierre Peterlongo on 02/08/2012.

using namespace std;
using namespace gatb::core::tools::dp;
using namespace gatb::core::tools::dp::impl;
using namespace gatb::core::system;
using namespace gatb::core::system::impl;
using namespace gatb::core::tools::misc;

#define DEBUG(a)  //printf a

#define BUFFER_SIZE     (256*1024)

#define nearest_power_of_2(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

/********************************************************************************/
namespace gatb {  namespace core {  namespace bank {  namespace impl {
/********************************************************************************/

/********************************************************************************/
// heavily inspired by kseq.h from Heng Li (https://github.com/attractivechaos/klib)
typedef struct
{
    gzFile stream;
    unsigned char *buffer;
    int buffer_start, buffer_end;
    bool eof;
    char last_char;

    void rewind ()
    {
        gzrewind (stream);
        last_char = eof = buffer_start = buffer_end = 0;
    }

} buffered_file_t;

/********************************************************************************/
struct variable_string_t
{
     variable_string_t ()  : length(0), max(0), string(0) {}
    ~variable_string_t () { if (string!=0)  { FREE(string); } }

    int length, max;
    char *string;
};

/********************************************************************************/
struct buffered_strings_t
{
     buffered_strings_t () : read(new variable_string_t), dummy(new variable_string_t), header(new variable_string_t)  {}
    ~buffered_strings_t ()
    {
        delete read;
        delete dummy;
        delete header;
    }

    variable_string_t *read, *dummy, *header;
};

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Bank (const std::vector<std::string>& filenames)
    : _filenames(filenames), filesizes(0), nb_files(0), _insertHandle(0)
{
    init ();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Bank (int argc, char* argv[])
    : filesizes(0), nb_files(0), _insertHandle(0)
{
    for (size_t i=0; i<argc; i++)  { _filenames.push_back (argv[i]); }

    init ();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Bank (const std::string& filename)
    : filesizes(0), nb_files(0), _insertHandle(0)
{
    _filenames.push_back (filename);
    init ();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::~Bank ()
{
    if (_insertHandle != 0)  { fclose (_insertHandle); }
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::init ()
{
    /** We check that we don't exceed the number of allowed files. */
    if (_filenames.empty() || _filenames.size() > getMaxNbFiles())
    {
        /** We send an exception. */
        throw gatb::core::system::Exception (STR_BANK_bad_file_number, _filenames.size(), getMaxNbFiles());
    }

    nb_files  = _filenames.size();
    filesizes = 0;

    // estimate total size of files
    for (size_t i=0; i<nb_files; i++)
    {
        /** Shortcut. */
        const char* fname = _filenames[i].c_str();

        bool compressed = false;
        u_int64_t estimated_filesize;

        if (strstr (fname, "gz") == (fname + strlen (fname) - 2))
            compressed = true;

        if (compressed)
            // crude hack, based on Quip paper reporting compression ratio (~0.3).
            // gzseek(SEEK_END) isn't supported. need to read whole file otherwise :/

            estimated_filesize = System::file().getSize (fname) * 4;
        else
            estimated_filesize = System::file().getSize (fname);

        filesizes += estimated_filesize;
    }
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
u_int64_t Bank::estimateNbSequences ()
{
    /** We create an iterator for the bank. */
    Bank::Iterator it (*this, Iterator::NONE);

    /** We return the estimation of sequences number. */
    return it.estimateNbSequences();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
size_t Bank::estimateMaxSequenceLength ()
{
    size_t max_readlen = 0;

    /** We need an iterator on the bank. */
    Bank::Iterator it (*this);

    /** We truncate the iterator. */
    TruncateIterator<Sequence> itTrunc (it, 10000);

    /** We loop the sequences and compute the maximum length. */
    for (it.first(); !it.isDone(); it.next())
    {
        max_readlen = max (it->getDataSize(), max_readlen);
    }

    /** We return the result. */
    return max_readlen;
}


/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::insert (const Sequence& item)
{
    /** We open the last file if needed. */
    if (_insertHandle == 0  &&  _filenames.empty()==false)
    {
        _insertHandle = fopen (_filenames[_filenames.size()-1].c_str(), "w0h");
    }

    if (_insertHandle != 0)
    {
        /** We add the sequence into the bag. */
        fprintf (_insertHandle, ">%s\n", item.getComment());
        fprintf (_insertHandle, "%s\n", item.getDataBuffer());
    }
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Iterator::Iterator (Bank& ref, CommentMode_e commentMode)
    : _ref(ref), _commentsMode(commentMode), index_file(0), buffered_file(0), buffered_strings(0)
{
    /** We initialize the iterator. */
    init  ();

    /** We go to the first item (if any). */
	// first ();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Iterator::~Iterator ()
{
    finalize ();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::Iterator::first()
{
    for (int i = 0; i < _ref.nb_files; i++)
    {
        buffered_file_t* bf = (buffered_file_t *) buffered_file[i];
        if (bf != 0)  { bf->rewind(); }
    }

    index_file = 0;

    next();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::Iterator::next()
{
    if (_commentsMode == NONE)
    {
        _isDone = get_next_seq (_item->getData()) == false;
    }
    else
    {
        _isDone = get_next_seq (_item->getData(), &_item->comment, &_item->commentSize, _commentsMode) == false;
    }
    DEBUG (("Bank::Iterator::next  _isDone=%d\n", _isDone));
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
// the following functions are adapted from kseq.h by Heng Li (https://github.com/attractivechaos/klib)
inline bool rebuffer (buffered_file_t *bf)
{
    if (bf->eof) return false;
    bf->buffer_start = 0;
    bf->buffer_end = gzread (bf->stream, bf->buffer, BUFFER_SIZE);
    if (bf->buffer_end < BUFFER_SIZE) bf->eof = 1;
    if (bf->buffer_end == 0) return false;
    return true;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
inline signed char buffered_getc (buffered_file_t *bf)
{
    if (bf->buffer_start >= bf->buffer_end) if (!rebuffer (bf)) return -1;
    return (signed char) (bf->buffer[bf->buffer_start++]);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
inline signed int buffered_gets (
    buffered_file_t*   bf,
    variable_string_t* s,
    char *dret,
    bool append,
    bool allow_spaces
)
{
    if (dret) *dret = 0;
    if (!append) s->length = 0;
    if (bf->buffer_start >= bf->buffer_end && bf->eof) return -1;
    while (1)
    {
        int i;
        if (bf->buffer_start >= bf->buffer_end) if (!rebuffer (bf)) break;
        if (allow_spaces)
        {
            for (i = bf->buffer_start; i < bf->buffer_end; i++)
                if (bf->buffer[i] == '\n') break;
        }
        else
        {
            for (i = bf->buffer_start; i < bf->buffer_end; i++)
                // isspace() answers yes for ' ', \t, \n, \v, \f, \r
                if (isspace (bf->buffer[i])) break;
        }
        if (s->max - s->length < (i - bf->buffer_start + 1))
        {
            s->max = s->length + (i - bf->buffer_start + 1);
            nearest_power_of_2(s->max);
            s->string = (char*)  REALLOC (s->string, s->max);
        }
         memcpy (s->string + s->length, bf->buffer + bf->buffer_start, i - bf->buffer_start);
        s->length += i - bf->buffer_start;
        bf->buffer_start = i + 1;
        if (i < bf->buffer_end)
        {
            if (dret) *dret = bf->buffer[i];
            break;
        }
    }
    if (s->string == NULL)
    {
        s->max = 256;
        s->string = (char*)  CALLOC (256, 1);
    }
    else if (allow_spaces && s->length > 1 && s->string[s->length - 1] == '\r')
        s->length--;
    s->string[s->length] = '\0';
    return s->length;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
bool Bank::Iterator::get_next_seq_from_file (Vector<char>& data, char **cheader, int *hlen, int file_id, CommentMode_e mode)
{
    buffered_strings_t* bs = (buffered_strings_t*) buffered_strings;

    signed char c;
    buffered_file_t *bf = (buffered_file_t *) buffered_file[file_id];
    if (bf->last_char == 0)
    {
        while ((c = buffered_getc (bf)) != -1 && c != '>' && c != '@')
            ; // go to next header
        if (c == -1) return false; // eof
        bf->last_char = c;
    }
    bs->read->length = bs->dummy->length = 0;

    if (buffered_gets (bf, bs->header, (char *) &c, false, false) < 0) //ici
        return false; // eof

    if (c != '\n')
    {
        if (mode == IDONLY)
        {
            variable_string_t dummy;
            buffered_gets (bf, &dummy, NULL, true, true); // read header
        }
        else
        {
            // We add the last read character (likely a space)
            bs->header->string[bs->header->length] = c;
            bs->header->length++;
            bs->header->string[bs->header->length] = 0;

            buffered_gets (bf, bs->header, NULL, true, true); // read header
        }
	}

    if (bs->read->string == NULL)
    {
        bs->read->max = 256;
        bs->read->string = (char*)  MALLOC (bs->read->max);
    }
    while ((c = buffered_getc (bf)) != -1 && c != '>' && c != '+' && c != '@')
    {
        if (c == '\n') continue; // empty line
        bs->read->string[bs->read->length++] = c;
        buffered_gets (bf, bs->read, NULL, true, true);
    }
    if (c == '>' || c == '@') bf->last_char = c;
    if (bs->read->length + 1 >= bs->read->max)
    {
        bs->read->max = bs->read->length + 2;
        nearest_power_of_2(bs->read->max);
        bs->read->string = (char*)  REALLOC (bs->read->string, bs->read->max);
    }
    bs->read->string[bs->read->length] = '\0';
    if (c == '+') // fastq
    {
        if (bs->dummy->max < bs->read->max) // resize quality to match read length
        {
            bs->dummy->max = bs->read->max;
            bs->dummy->string = (char*)  REALLOC (bs->dummy->string, bs->dummy->max);
        }
        while ((c = buffered_getc (bf)) != -1 && c != '\n')
            ; // read rest of quality comment
        while (buffered_gets (bf, bs->dummy, NULL, true, true) >= 0 && bs->dummy->length < bs->read->length)
            ; // read rest of quality
        bf->last_char = 0;
    }

    /** We update the data of the sequence. */
    data.set (bs->read->string, bs->read->length);

    if (cheader && hlen)
    {
        *cheader = bs->header->string;
        *hlen    = bs->header->length;
    }

    return true;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
bool Bank::Iterator::get_next_seq_from_file (Vector<char>& data, int file_id)
{
    return get_next_seq_from_file (data, NULL, NULL, file_id, NONE);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
bool Bank::Iterator::get_next_seq (Vector<char>& data, char **cheader, int *hlen, CommentMode_e mode
)
{
    bool success = get_next_seq_from_file (data, cheader, hlen, index_file, mode);
    if (success) return true;

    // cycle to next file if possible
    if (index_file < _ref.nb_files - 1)
    {
        index_file++;
        return get_next_seq (data, cheader, hlen, mode);
    }
    return false;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
bool Bank::Iterator::get_next_seq (Vector<char>& data)
{
    return get_next_seq (data, NULL, NULL, NONE);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::Iterator::init ()
{
    /** We initialize the array of files. */
    buffered_file = (void**) calloc (getMaxNbFiles(), sizeof(void*));

    /** Shortcut. */
    vector<string>& fnames = _ref._filenames;

    // open each file for reading
    for (size_t i=0; i<_ref.nb_files; i++)
    {
        /** Shortcut. */
        const char* fname = fnames[i].c_str();

        buffered_file_t** bf = (buffered_file_t **) buffered_file + i;
        *bf = (buffered_file_t *)  CALLOC (1, sizeof(buffered_file_t));
        (*bf)->buffer = (unsigned char*)  MALLOC (BUFFER_SIZE);
        (*bf)->stream = gzopen (fname, "r");

        /** We check that we can open the file. */
        if ((*bf)->stream == NULL)
        {
            /** We first try do do some cleanup. */
            finalize ();

            /** We launch an exception. */
            throw gatb::core::system::ExceptionErrno (STR_BANK_unable_open_file, fname);
        }
    }

    index_file = 0; // initialize the get_next_seq iterator to the first file

    // init read and dummy (for readname and quality)
    buffered_strings_t* bs = new buffered_strings_t;
    buffered_strings = bs;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::Iterator::finalize ()
{
    buffered_strings_t* bs = (buffered_strings_t*) buffered_strings;

    if (bs != 0)  { delete bs; }

    for (int i = 0; i < _ref.nb_files; i++)
    {
        buffered_file_t* bf = (buffered_file_t *) buffered_file[i];

        if (bf != 0)
        {
            /** We close the handle of the file. */
            if (bf->stream != NULL)  {  gzclose (bf->stream);  }

            /** We delete the buffer. */
            FREE (bf->buffer);

            /** We delete the buffered file itself. */
            FREE (bf);
        }
    }

    /** We release the array of files. */
    free (buffered_file);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
u_int64_t Bank::Iterator::estimateNbSequences ()
{
    Vector<char> data;

    /** We initialize the iterator (and so points to the first item if any). */
    first ();

    /** We count at least one sequence.*/
    u_int64_t volume = 1;
    while (get_next_seq (data))
    {
        if (++volume == 1000) break;
    }

    u_int64_t actualPosition = 0;

    /** We compute the aggregated size from the files having been read until we
     * reached our limit number of sequences. */
    for (size_t i=0; i<=index_file; i++)
    {
        buffered_file_t* current = (buffered_file_t *) buffered_file[i];

        actualPosition += gztell (current->stream);
    }

    if (actualPosition > 0)
    {
        volume = (volume * _ref.getSize()) / actualPosition; // linear extrapolation from the first 1k reads
    }

    /** We return the estimation. */
    return volume;
}

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/
