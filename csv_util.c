/******************************************************************************
 * CSV File Parser Routines
 *
 * This file contains routines for parsing the CSV files and extracting the
 * fields from the scanned lines.
 *
 * Assumptions:
 *
 * This parser makes the following assumptions
 *
 * Revision History:
 *
 *	Version 1.0 (27/Sep/2004)  Sajith P V
 *		Initial Release
 *
 * Copyright (C) CarrierComm Inc.
 ******************************************************************************/

/******************************************************************************
 *								Include Files
 ******************************************************************************/

/* Standard Library Includes */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Module Includes */
#include "csv_util.h"

/******************************************************************************
 *								  Variables
 ******************************************************************************/

/*
 * Array to store the error strings for various parser error codes
 */

#define CSV_ERR_CODE_INIT(Code, Name) Name,
char *parse_errors[] =
{
	CSV_PARSE_ERROR_CODES
};
#undef CSV_ERR_CODE_INIT

/******************************************************************************
 *						Macros, Typedefs & Enumerations
 ******************************************************************************/

/*
 * These macros are used to turn ON and OFF
 * flags to indicate '"' character in the
 * scanned line. Any cell in a csv file
 * containing comma separated items will
 * be enclosed in a pair of quotes. These
 * defines are used to sniff them out.
 */

#define YEP_HIT_IT_ALRIGHT		1	/* Used when starting quotes is found */
#define NOPE_NO_QUOTES_YET		0	/* Used when ending quotes is found */

/******************************************************************************
 *							Function Definitions
 ******************************************************************************/

/******************************************************************************
 * Description:	Re-sizes the memory pointed by 'ptr' to 'size' bytes.
 *
 * Inputs:      ptr - memory to be readjusted.
 *				size - new value of the size for memory pointed by ptr
 *
 * Outputs: 	Pointer to readjusted memory.
 ******************************************************************************/
void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if(ptr == NULL && size != 0)
	{
		return (void *)(-E_OUT_OF_MEM);
	}

	return ptr;
} /* end of xrealloc */

/******************************************************************************
 * Description:	Converts integer given as argument to string.
 *
 * Inputs:      i - integer to be converted
 *				base	- base value for conversion
 *
 * Outputs:  	Pointer to the ascii string is returned
 ******************************************************************************/
char *itoa(int i, char base)
{
	#define MAX_CHARS  12

    static char a[MAX_CHARS];
    char *b = a + sizeof(a) - 1;
    int sign = (i < 0);

    if(sign)
    {
		i = -i;
	}

    *b = 0;
    do
    {
		*--b = '0' + (i % base);
		i /= base;
    }while(i);

    if (sign)
    {
		*--b = '-';
	}

    return b;
} /* end of itoa */

/******************************************************************************
 * Description:	Utility function to print the string equivalent of the error
 *				code given.
 *
 * Inputs:      errnum - error code to be printed
 *
 * Outputs:  	Error string is printed onto the console.
 ******************************************************************************/
void print_parse_error(int errnum)
{
	int err;

	if(errnum < 0)
	{
		errnum *= (-1);
	}

	err = errnum - 1;

	if((err>=0) && (err<MAX_CSV_PARSE_ERROR_CODES))
	{
		printf("Error (%d) : %s\n",err+1, parse_errors[err]);
	}
	else
	{
		printf("Error encountered\n");
	}
} /* end of print_parse_error */

/******************************************************************************
 * Description:	Utility function to return the data from the specified field
 *				(column) of the specified line from the csv file.
 *
 * Inputs:      csv_fp    - file pointer to the csv file
 *				line_num  - line from which the given field is to be read. If
 *							0 returns the field data of line read from current
 *							file pointer. Line number starts from 1.
 *				field_num - the field to be read. Field number starts from 1
 *				data      - field data is returned in this pointer. Sufficient
 *							memory must be allocated by the calling application
 *
 * Outputs: 	0  - Success. The read field is returned in 'data'
 *				-ve error code on failure
 ******************************************************************************/
int get_field_data(FILE *csv_fp, int line_num, int field_num, char *data)
{
	char 	fields[MAX_CSV_COLS][MAX_CSV_COL_SIZE];
	int		num_fields=MAX_CSV_COLS;
	int		retval=0;

	/*
	 * Validate all inputs
	 */

	if(!csv_fp)
	{
		return -E_INV_FP;
	}
	else if(line_num < 0)
	{
		return -E_INV_LINE_NUM;
	}
	else if(((field_num-1) < 0) || ((field_num-1) >= MAX_CSV_COLS))
	{
		return -E_INV_FIELD_NUM;
	}
	else if(!data)
	{
		return -E_INV_PTR;
	}

	/* Set all elements of 'fields' array to NULL */
	memset(fields, 0, sizeof(fields));

	/* If line number is not zero, rewind the file pointer */
	if(line_num)
	{
		rewind(csv_fp);
	}

	if((retval = read_csv_line(csv_fp, fields, &num_fields, line_num)) < 0)
	{
		return retval;
	}

	/*
	 * Blank the data field in case the probed cell don't have any
	 * characters in it.
	 */

	*data = '\0';

	if(fields[field_num-1])
	{
		strncpy(data, fields[field_num-1],MAX_CSV_COL_SIZE);

		/*
		 * If length of fields[field_num] > MAX_CSV_COL_SIZE
		 * strncpy will not null terminate !!!
		 */

		*(data + MAX_CSV_COL_SIZE - 1) = '\0';
	}

	return 0;
}

/******************************************************************************
 * Description:	Utility function to take the file pointer to the beginning of
 *				the given section. Sections are assumed to be delimited by
 *				section-strings. It must be noted that section names should be
 *				in the first column of the given csv file and no other row
 *				in the file should have any of the section-names.
 *
 * Inputs:      csv_fp  - file pointer to the csv file
 *				section - section identifying string
 *
 * Outputs:  	0  - Success
 *				-ve error code on failure
 ******************************************************************************/
int get_to_section(FILE *csv_fp, char *section)
{
	char 	field_data[MAX_CSV_COL_SIZE + 1];
	int		retval = -E_SECTION_NOT_FOUND;

	if(!csv_fp)
	{
		return -E_INV_FP;
	}

	if(!section)
	{
		return -E_INV_PTR;
	}

	rewind(csv_fp);

	while(get_field_data(csv_fp, 0, 1, field_data) >= 0)
	{
		/* Did we hit the section ? */
		if(!strcasecmp(field_data, section))
		{
			/* Yep, we sure did..breaking out.. */
			retval = 0;
			break;
		}
	}

	return retval;
} /* end of get_to_section */

/******************************************************************************
 * Description:	Utility routine to remove leading & terminating white spaces
 *				in the given string. White spaces within the string will be
 *				untouched !!!
 *
 * Inputs:      str - string to be processed
 *
 * Outputs:  	The input string is modified and its pointer is also returned,
 *				this enables the function to be used in a construct such as
 *				printf("%s\n",dos_to_unix(str));
 ******************************************************************************/
char *eat_white_space(char *str)
{
	char *p;

	if(str)
	{
		p = str;

		/*
		 * Plough through all white spaces to reach first non-white
		 * space character
		 */

		while(*p && isspace((int)*p))
		{
			p++;
		}

		/*
		 * Move all non-white space characters to overwrite the white
		 * spaces
		 */
		strcpy(str, p);

		p = str;

		/* Set the pointer to end of string */
		while(*p++);

		p = p - 1;

		/* Eat all terminating white spaces */
		if(p)
		{
			while(isspace((int)*--p));
			*(p+1) = '\0';
		}
	}

	return str;
} /* end of eat_white_space */

/******************************************************************************
 * Description:	Utility routine to remove '\n' character from the end of a
 *				given string. This can be used after a fgets() call.
 *
 * Inputs:      str - string to be processed
 *
 * Outputs:  	The input string is modified and its pointer is also returned,
 *				this enables the function to be used in a construct such as
 *				printf("%s\n",dos_to_unix(str));
 ******************************************************************************/
char *eat_terminating_newline(char *str)
{
	char *ptr=str;

	if(ptr)
	{
		/*
		 * Rip through until the terminating null
		 * character
		 */

		while(*ptr)
		{
			ptr++;
		}

		/*
		 * Replace '\n' with '\0' only if its there!!!
		 */

		if(*(ptr-1) == '\n')
		{
			*(ptr-1) = '\0';
		}
	}

	return str;
} /* end of eat_terminating_newline */

/******************************************************************************
 * Description:	Utility routine to replace a sequence of "\r\n" or "\n\r" in
 *				the given string with "\n". This function assumes that str is
 *				a null terminated string.
 *
 * Inputs:      str - string to be processed
 *
 * Outputs:	    The input string is modified and its pointer is also returned,
 *				this enables the function to be used in a construct such as
 *				printf("%s\n",dos_to_unix(str));
 ******************************************************************************/
char *dos_to_unix(char *str)
{
	char *ptr=str;

	if(ptr)
	{
		while(*ptr)
		{
			if(((*ptr == '\r') && (*(ptr+1) == '\n')) ||
				((*ptr == '\n') && (*(ptr+1) == '\r')))
			{
				*ptr = '\n';

				/* Now shift the whole string to left */
				char *temp_ptr=ptr+1;
				while(*temp_ptr)
				{
					*temp_ptr=*(temp_ptr+1);
					temp_ptr++;
				}
			}
			ptr++;
		}
	}

	return str;
} /* end of dos_to_unix */

/******************************************************************************
 * Description:	This function 'malloc's memory to hold the input string,copies
 *				the string to the newly allocated memory and returns the
 *				starting address of the allocated memory.
 *
 * Inputs:      field - string to be duplicated
 *				max_size - max bytes to be copied to the new string from
 *						   the source string
 *
 * Outputs: 	0	-	Success
 *				-1	-	Failure
 *
 ******************************************************************************/
char *grab_field_mem(char *field, int max_size)
{
	char *field_mem=(char *)NULL;

	if(field)
	{
		/* Get memory to store the field */
		if((field_mem = malloc(strlen(field))) != NULL)
		{
			strncpy(field_mem, field, max_size);

			*(field_mem + max_size - 1) = '\0';
		}
	}

	return field_mem;
} /* end of grab_field_mem */

/******************************************************************************
 * Description:	This utility reads the line whose line number is given as
 *				argument in 'line_num'. If line_num is 0, it just reads the
 *				line from the current file pointer and returns it.
 *
 * Inputs:      csv_fp 	- 	file pointer to the file being read
 *				line 	- 	receptacle into which line is to be read. Memory
 *							must be	allocated by the calling function.
 *				line_size - maximum number of bytes to be read into 'line'
 *				line_num  - 0 means read line from current file pointer, +ve
 *						    value means the line number of the line to be read
 *							from the file
 *
 *
 * Outputs: 	Pointer to string read on success
 *				(char *)NULL - Failure
 ******************************************************************************/
char *read_line(FILE *csv_fp, char *line, int line_size, int line_num)
{
	int i;

	if(line && csv_fp && (line_num >= 0))
	{
		if(line_num == 0)
		{
			return (fgets(line, line_size, csv_fp));
		}
		else
		{
			rewind(csv_fp);

			for(i=1; i <= (line_num-1); i++)
			{
				while(1)
				{
					/* Read first (line_num-1) lines and discard the contents */
					if(fgets(line, line_size, csv_fp) == NULL)
					{
						return ((char *)NULL);
					}

					if(line[strlen(line)-1] == '\n')
					{
						break;
					}
				}
			}

			/* Read the desired line now */
			return (fgets(line, line_size, csv_fp));
		}
	}/* end of if(line) */
	else
	{
		return ((char *)NULL);
	}
} /* end of read_line */

/******************************************************************************
 * Description:	Read a line from the given file, break it up into fields and
 *				return the fields in the char array 'fields'. Space for fields
 *				will be allocated in this function and must be freed by the
 *				calling application !!! The number of fields read is returned
 *				in num_fields.
 *
 *				Design considerations for this function are as follows:
 *					a. Ensure that not more than the requested number of
 *					   fields are read.
 *					b. Not more than MAX_CSV_COL_SIZE bytes will be read into
 *					   the memory pointed to by pointers in 'fields' array.
 *
 * Inputs:      csv_fp 	- 	file pointer to the file being read
 *				fields 	- 	array of char pointers into which the allocated
 *							buffer addresses are to be stored.
 *				num_fields - total number of fields to be read from the line.
 *							 This is a value-result argument. On return, this
 *							 variable contains actual number of fields read
 *				line_num  - 0 means read line from current file pointer, +ve
 *						    value means the line number of the line to be read
 *							from the file
 *
 *
 * Outputs:  	0 - success
 *				-ve error code on failure
 ******************************************************************************/
int read_csv_line(FILE	*csv_fp, char fields[][MAX_CSV_COL_SIZE], int *num_fields, int line_num)
{
	char 	line[MAX_LINE_SIZE];/* temporary buffer to store line read from
								 * file
								 */
	char	*field;				/* to store a field at a time */
	char 	*delim;				/* to search for field delimiters */

	/*
	 * 'field' is a value-result argument. It gives the
	 * maximum number of fields expected from the line that is
	 * read in. This value is saved in the following variable,
	 * 'max_fields'. On returning from this function, num_field
	 * would have been updated with the number of fields actually
	 * read
	 */

	int		max_fields;

	/*
	 * This variable is used to handle embedded '"' character within
	 * a cell of csv file. A cell containing embedded ',' will be
	 * enclosed within a pair of '"'. This whole bunch must be treated
	 * as a single field and not separate !!! 'hit_the_quote' is set
	 * to 1 on encountering the first '"' and reset to 0 on getting
	 * to the second '"'
	 */

	char	hit_the_quote;

	if(num_fields && csv_fp)
	{
		max_fields = *num_fields;
	}
	else
	{
		return -E_INV_PTR;
	}

	if(*num_fields <= 0)
	{
		return -E_INV_FIELD_NUM;
	}

	/*
	 * Clear num_fields. As and when a field is read in
	 * num_fields is incremented to reflect the same.
	 */

	*num_fields = 0;

	/*
	 * Read in a line from the input csv file
	 */

	if(read_line(csv_fp, line, sizeof(line), line_num) != NULL)
	{
		/* Clean the line */
		eat_white_space(eat_terminating_newline(dos_to_unix(line)));

		/*
		 * Break down this line into fields !!!
		 */

		delim = field = line;
		hit_the_quote = 0;

		while(1)
		{
			switch(*delim)
			{
				case '\0':
					if(strlen(field) != 0)
					{
						strncpy(fields[*num_fields], field, MAX_CSV_COL_SIZE);

						fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

						(*num_fields)++;
					}
					return 0;

				case ',':
					/*
					 * If we had previously hit a '"' this ',' is
					 * belongs to the same field. Hence its not a
					 * delimiter
					 */

					if(!hit_the_quote)
					{
						/*
						 * Eliminate the delimiter with '\0' to get
						 * a field
						 */

						*delim = '\0';

						if(strlen(field) != 0)
						{
							strncpy(fields[*num_fields], field,
								MAX_CSV_COL_SIZE);

							fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

							(*num_fields)++;

							if((*num_fields) == max_fields)
							{
								return 0;
							}
						}
						field = delim+1;
					}

					break;

				case '"':
					/*
					 * Check to see if we have already hit the
					 * first quote
					 */

					if(hit_the_quote)
					{
						hit_the_quote = NOPE_NO_QUOTES_YET;
						*delim = '\0';

						if(strlen(field) != 0)
						{
							strncpy(fields[*num_fields], field,
								MAX_CSV_COL_SIZE);

							fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

							(*num_fields)++;

							if((*num_fields) == max_fields)
							{
								return 0;
							}
						}
						else
						{
							/* We have come across quotes within the cell */
							return -E_QUOTES_FOUND;
						}
						field = delim + 1;
						break;
					}
					else
					{
						/*
						 * Check to see if we are hitting quote for
						 * the first time
						 */

						hit_the_quote = YEP_HIT_IT_ALRIGHT;
						field = delim + 1;
					}
					break;

				default:
					break;

			} /* end of switch(*delim) */

			delim++;
		}/* end of while there are chars to be read in line */

	} /* end of if a line was read */
	else
	{
		return -E_EOF;
	}

	return 0;
} /* end of read_line */


/******************************************************************************
 * Description:	This utility reads the line whose line number is given as
 *				argument in 'line_num'. If line_num is 0, it just reads the
 *				line from the current file pointer and returns it.
 *
 * Inputs:      csv_fp 	- 	file pointer to the file being read
 *				line 	- 	receptacle into which line is to be read. Memory
 *							must be	allocated by the calling function.
 *				line_size - maximum number of bytes to be read into 'line'
 *				line_num  - 0 means read line from current file pointer, +ve
 *						    value means the line number of the line to be read
 *							from the file
 *
 *
 * Outputs: 	Pointer to string read on success
 *				(char *)NULL - Failure
 ******************************************************************************/
char *read_next_line(FILE *csv_fp, char *line, int line_size)
{

	if(line && csv_fp)
	{
		while(1)
		{
			/* Read first (line_num-1) lines and discard the contents */
			if(fgets(line, line_size, csv_fp) == NULL)
			{
				return ((char *)NULL);
			}

			if(line[strlen(line)-1] == '\n')
			{
				break;
			}
		}

	}/* end of if(line) */
	else
	{
		return ((char *)NULL);
	}
	return line;

} /* end of read_line */
/******************************************************************************
 * Description:	Read a line from the given file, break it up into fields and
 *				return the fields in the char array 'fields'. Space for fields
 *				will be allocated in this function and must be freed by the
 *				calling application !!! The number of fields read is returned
 *				in num_fields.
 *
 *				Design considerations for this function are as follows:
 *					a. Ensure that not more than the requested number of
 *					   fields are read.
 *					b. Not more than MAX_CSV_COL_SIZE bytes will be read into
 *					   the memory pointed to by pointers in 'fields' array.
 *
 * Inputs:      csv_fp 	- 	file pointer to the file being read
 *				fields 	- 	array of char pointers into which the allocated
 *							buffer addresses are to be stored.
 *				num_fields - total number of fields to be read from the line.
 *							 This is a value-result argument. On return, this
 *							 variable contains actual number of fields read
 *				line_num  - 0 means read line from current file pointer, +ve
 *						    value means the line number of the line to be read
 *							from the file
 *
 *
 * Outputs:  	0 - success
 *				-ve error code on failure
 ******************************************************************************/
int read_csv_next_line(FILE	*csv_fp, char fields[][MAX_CSV_COL_SIZE], int *num_fields, int line_num)
{
	char 	line[MAX_LINE_SIZE];/* temporary buffer to store line read from
								 * file
								 */
	char	*field;				/* to store a field at a time */
	char 	*delim;				/* to search for field delimiters */

	/*
	 * 'field' is a value-result argument. It gives the
	 * maximum number of fields expected from the line that is
	 * read in. This value is saved in the following variable,
	 * 'max_fields'. On returning from this function, num_field
	 * would have been updated with the number of fields actually
	 * read
	 */

	int		max_fields;

	/*
	 * This variable is used to handle embedded '"' character within
	 * a cell of csv file. A cell containing embedded ',' will be
	 * enclosed within a pair of '"'. This whole bunch must be treated
	 * as a single field and not separate !!! 'hit_the_quote' is set
	 * to 1 on encountering the first '"' and reset to 0 on getting
	 * to the second '"'
	 */

	char	hit_the_quote;


	if(num_fields && csv_fp)
	{
		max_fields = *num_fields;
	}
	else
	{
		return -E_INV_PTR;
	}

	if(*num_fields <= 0)
	{
		return -E_INV_FIELD_NUM;
	}

	/*
	 * Clear num_fields. As and when a field is read in
	 * num_fields is incremented to reflect the same.
	 */

	*num_fields = 0;

	/*
	 * Read in a line from the input csv file
	 */

	if(read_next_line(csv_fp, line, sizeof(line))!=NULL)
	{
		/* Clean the line */
		eat_white_space(eat_terminating_newline(dos_to_unix(line)));

		/*
		 * Break down this line into fields !!!
		 */

		delim = field = line;
		hit_the_quote = 0;



		while(1)
		{
			switch(*delim)
			{
				case '\0':
					if(strlen(field) != 0)
					{
						strncpy(fields[*num_fields], field, MAX_CSV_COL_SIZE);

						fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

						(*num_fields)++;
					}


					return 0;

				case ',':
					/*
					 * If we had previously hit a '"' this ',' is
					 * belongs to the same field. Hence its not a
					 * delimiter
					 */

					if(!hit_the_quote)
					{
						/*
						 * Eliminate the delimiter with '\0' to get
						 * a field
						 */

						*delim = '\0';

						strncpy(fields[*num_fields], field,
								MAX_CSV_COL_SIZE);

						fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

						(*num_fields)++;

						if((*num_fields) == max_fields)
						{
							return 0;
						}

						field = delim+1;
					}

					break;

				case '"':
					/*
					 * Check to see if we have already hit the
					 * first quote
					 */

					if(hit_the_quote)
					{
						hit_the_quote = NOPE_NO_QUOTES_YET;
						*delim = '\0';

						strncpy(fields[*num_fields], field,
								MAX_CSV_COL_SIZE);

						fields[*num_fields][MAX_CSV_COL_SIZE-1] = '\0';

						(*num_fields)++;

						if((*num_fields) == max_fields)
						{
							return 0;
						}

						field = delim + 1;
						break;
					}
					else
					{
						/*
						 * Check to see if we are hitting quote for
						 * the first time
						 */

						hit_the_quote = YEP_HIT_IT_ALRIGHT;
						field = delim + 1;
					}
					break;

				default:
					break;

			} /* end of switch(*delim) */

			delim++;
		}/* end of while there are chars to be read in line */

	} /* end of if a line was read */
	else
	{
		return -E_EOF;
	}

	return 0;
} /* end of read_line */


/******************************************************************************
 *								  File Ends
 ******************************************************************************/
