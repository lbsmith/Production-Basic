/*
 * $Id: errors.h,v 1.2 2000/01/27 14:41:28 lees Exp $
 */

#ifndef ERRORS_H
#define ERRORS_H

#define ERR_DIRECTIVE_NOT_AVAILABLE    -1
#define ERR_TIMEOUT                    0
#define ERR_END_OF_RECORD              1
#define ERR_END_OF_FILE                2
#define ERR_KEY_FIELD_NOT_FOUND        3
#define ERR_BAD1                       4
#define ERR_BAD2				 	   5
#define ERR_BAD3                       6
#define ERR_FILE_CORRUPTION            7
#define ERR_BAD4                       8
#define ERR_BAD5                       9

#define ERR_FILEID_SIZE_KEY_USAGE      10
#define ERR_MISSING_DUPLICATE_KEY      11
#define ERR_BAD_FILEID                 12
#define ERR_FILE_DEVICE_BUSY           13
#define ERR_FILE_DEVICE_USAGE          14
#define ERR_OUT_OF_DISK_SPACE          15
#define ERR_DISK_DIRECTORY_CAPACITY    16
#define ERR_INVALID_PARAMETER          17
#define ERR_ILLEGAL_PROG_ENCRYPTION    18
#define ERR_PROGRAM_FORMAT_INCORRECT   19

#define ERR_SYNTAX                     20
#define ERR_STATEMENT_NUMBER           21
#define ERR_UNINITIALIZED_VARIABLE     22
#define ERR_FUNC_NAME_ALREADY_EXISTS   24
#define ERR_UNDEFINED_FUNC             25
#define ERR_VARIABLE_USAGE             26
#define ERR_RETURN_WITHOUT_GOSUB       27
#define ERR_NEXT_WITHOUT_FOR           28
#define ERR_UNDEFINED_MNEMONIC         29

#define ERR_PROGRAM_CHECKSUM           30
#define ERR_INTERNAL_STACK_OVERFLOW    31
#define ERR_RECORD_TOO_LARGE           32
#define ERR_MEMORY_CAPACITY            33
#define ERR_FOR_GOSUB_STACK_OVERFLOW   34
#define ERR_LISTER_STACK_OVERFLOW      35
#define ERR_CALL_ENTER_MISMATCH        36
#define ERR_FORMAT_TABLE_OVERFLOW      37
#define ERR_ILLEGAL_COMMAND            38
#define ERR_ESCAPE                     39

#define ERR_NUM_VALUE_OVERFLOW         40
#define ERR_INTEGER_RANGE              41
#define ERR_NONEXISTENT_SUBSCRIPT      42
#define ERR_NUM_FORMAT_MASK_OVERFLOW   43
#define ERR_STEP_SIZE_ZERO             44
#define ERR_STATEMENT_USAGE            45
#define ERR_STRING_SIZE                46
#define ERR_INVALID_SUBSTRING_REF      47
#define ERR_INPUT_VERIFICATION         48
#define ERR_GLOBAL_VARIABLE            49
#define ERR_CANNOT_REMOVE_PRIMARY_SORT 50
#define ERR_SORTS_IN_MSORT             51
#define ERR_SORTS_IN_TISAM             52
#define ERR_TOO_MANY_SEGMENT_DEFS      53
#define ERR_PRIMARY_KEY_MUST_BE_UNIQUE 54
#define ERR_SORT_NAME_TOO_LONG         55
#define ERR_FIELD_NUMBER               56
#define ERR_UNDEFINED_MODE             57
#define ERR_FIELD_DOES_NOT_EXIST       58
#define ERR_BAD6                       59

#define ERR_TRANS_LOG_NOT_FOUND        60
#define ERR_TRANS_IN_PROGRESS          61
#define ERR_TRANS_NOT_STARTED          62
#define ERR_TRANS_LOG_ALREADY_OPENED   63
#define ERR_TRANS_CHANNEL_NOT_OPENED   64
#define ERR_TRANS_PROGRESS_FILE_EXISTS 65

#endif
