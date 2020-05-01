#include <stdio.h>
#include <string.h>

/*
	For C-programs the header iso646.h needs to be included for full compability.
	For C++-programs this is included by default.
*/

const size_t buf_size = 2048;

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Usage: ./iso646 infile outfile\nOptional: --insert_header inserts the iso646 header at start of file.\n");
		return 0;
	}

	FILE *fin, *fout;
	if ((fin = fopen(*(argv + 1), "r")) == NULL)
	{
		printf("Error opening file \"%s\"\n.", *(argv + 1));
		return 0;
	}
	if ((fout = fopen(*(argv + 2), "w")) == NULL)
	{
		printf("Error creating output file \"%s\"\n.", *(argv + 2));
		return 0;
	}
	if (argc == 4 && !strcmp(*(argv + 3), "--insert_header"))
			fputs("%:include <iso646.h>\n", fout);
	char linebuffer[buf_size + 1]; // padding for safe look-ahead

	// exclude string literals, comments, and variable names
	int multicomment = 0,
		single_quote = 0,
		double_quote = 0,
		i, j, k, match, last_writeout;

	const char *tokens[] = {"&", "|", "!", "^", "{", "}", "[", "]",
			"#", "##", "&&", "&=", "!=", "||", "|=", "^="},
		*alternative[] = { "bitand", "bitor", "not", "xor", "<%", "%>",
						"<:", ":>", "%:", "%:%:", "and", "and_eq",
						"not_eq", "or", "or_eq", "xor_eq"};
	while (fgets(linebuffer, buf_size, fin) != NULL)
	{
		last_writeout = 0;
		for (i = 0; i < buf_size; i++)
		{
			if (linebuffer[i] == '\\') // escape character - ignore next symbol
			{
				i++;
				continue;
			}
			if (linebuffer[i] == '\0') // end of line
				break;
			if (single_quote && linebuffer[i] == '\'') // end of single quote literal
				single_quote = 0;
			else if (double_quote && linebuffer[i] == '"') // end of double quote literal
				double_quote = 0;
			else if (multicomment && linebuffer[i] == '*' && linebuffer[i + 1] == '/') // end of multi-line comment
				multicomment = 0;
			else if (!(single_quote || double_quote || multicomment))
			{
				if (linebuffer[i] == '/' && linebuffer[i + 1] == '/') // single-line comment, ignore rest of the line
					break;
				if (linebuffer[i] == '\'') // entering a single quote literal
					single_quote = 1;
				else if (linebuffer[i] == '"') // entering a double quote literal
					double_quote = 1;
				else if (linebuffer[i] == '/' && linebuffer[i + 1] == '*') // entering a multi-line comment
					multicomment = 1;
				else
				{
					match = 0;
					for (j = 0; j < 16; j++)
					{
						if (linebuffer[i] == tokens[j][0])
						{
							if (j < 4 || j == 8) // initial tokens which might be followed by a second valid one
							{
								for (k = 9; k < 16; k++) // range of double-length special tokens
								{
									if (linebuffer[i] == tokens[k][0] && linebuffer[i + 1] == tokens[k][1])
									{
										// matched double token
										match = 1;
										// fputs trickery, replace old symbol with null-byte and write out from last offset
										linebuffer[i] = '\0';
										fputs(linebuffer + last_writeout, fout);
										fputs(alternative[k], fout);
										if (j >= 10) // make sure there is a space to the right of word expressions
											if (linebuffer[i + 2] != ' ')
												fputs(" ", fout);
										last_writeout = i + 2; // increment offset for partial strings
										i += 1;
										break;
									}
								}
							}
							if (!match)
							{
								// matched single
								linebuffer[i] = '\0';
								fputs(linebuffer + last_writeout, fout);
								fputs(alternative[j], fout);

								if (j < 4) // pad a space afterwards
									if (linebuffer[i + 1] != ' ')
										fputs(" ", fout);
								last_writeout = i + 1;

								break;
							}
						}
					}
				}
			}
		}
		fputs(linebuffer + last_writeout, fout); // write out remainding line
	}
	fclose(fin);
	fclose(fout);
}