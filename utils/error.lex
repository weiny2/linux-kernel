
%{

#define DEBUG		0

#define DESC_START	1
#define DESC_COMMENT	2
#define FIELD_START	3
#define FIELD_TYPE	4
#define FIELD_NAME	5
#define FIELD_BITS	6
#define FIELD_COMMENT	7
#define FIELD_END	8
%}

%s	CSR

%%

^\/\/\ .*_(err_sts|ERR_STS).*\ desc:.*	{BEGIN CSR; return DESC_START;}
<CSR>^\/\/\ .*				return DESC_COMMENT;
<CSR>typedef.*				;
<CSR>struct\ \{				return FIELD_START;
<CSR>uint64_t				return FIELD_TYPE;
<CSR>[_a-zA-Z0-9]+			return FIELD_NAME;
<CSR>[0-9]+;				return FIELD_BITS;
<CSR>\/\/\ .*				return FIELD_COMMENT;
<CSR>\}					{BEGIN 0; return FIELD_END;}

.					;
\n					;

%%


int yywrap(void)
{
	return 1;
}

void toupper_str(char *str)
{
	while ((*str) != '\0') {
		*str = (char)toupper((int)(*str));
		str++;
	}
}

char *replace_str(char *str, char *old, char *new)
{
	static char buf[4096];
	char *p;

	if(!(p = strstr(str, old)))
		return str;

	strncpy(buf, str, p-str);
	buf[p-str] = '\0';

	sprintf(buf+(p-str), "%s%s", new, p+strlen(old));

	return buf;
}

int main(void)
{
	FILE *out = NULL;
	char filename[256];
	char *p, dname[256], csrname[256];
	char fname[256], fcomment[4096];
	int i, fbits = 0, dbits = 0;
	int token;

#if DEBUG
	while (token=yylex()) {
		printf("token %d: %s\n", token, yytext);
	}

#else

next:
	token = yylex();
	if (token == DESC_START) {
		p = &yytext[3];
		if (strstr(p, "FXR_")) {
			strcpy(csrname, p);
		} else {
			strcpy(csrname, "FXR_");
			strcat(csrname, p);
		}

		p = strstr(csrname, " desc:");
		p[0] = '\0';
		toupper_str(csrname);

		strcpy(dname, replace_str(&csrname[4], "_ERR_STS", ""));

		dbits = 0;

		strcpy(filename, dname);
		strcat(filename, ".CSR");
		out = fopen(filename, "w");
		if (out == NULL)
			perror(filename);

		fprintf(out,"/*\n");
		fprintf(out," *%s\n", &yytext[2]);
	} else
	if (token == DESC_COMMENT) {
		fprintf(out," *%s\n", &yytext[2]);
	} else
	if (token == FIELD_START) {
		fprintf(out," */\n");
		fprintf(out,"    {\n");
		fprintf(out,"\t\"%s\",\n", dname);
		fprintf(out,"\t%s, ",
			replace_str(csrname, "STS", "EN_HOST"));
		fprintf(out,"%s,\n",
			replace_str(csrname, "STS", "FIRST_HOST"));
		fprintf(out,"\t%s, ", csrname);
		fprintf(out,"%s, ",
			replace_str(csrname, "STS", "CLR"));
		fprintf(out,"%s,\n",
			replace_str(csrname, "STS", "FRC"));
		fprintf(out,"\t{\n");
	} else
	if (token == FIELD_NAME) {
		strcpy(fname, yytext);
	} else
	if (token == FIELD_BITS) {
		fbits = atoi(yytext);
		fcomment[0] = '\0';
	} else
	if (token == FIELD_COMMENT) {
		strcat(fcomment, &yytext[2]);
	} else
	if (token == FIELD_TYPE || token == FIELD_END) {
		for (i = 0; i < fbits; i++) {
			fprintf(out,"\t\t{ /* bit %d */\n", dbits);
			fprintf(out,"\t\t\"%s\",\n", fname);
			fprintf(out,"\t\t\"%s\",\n", fcomment);
			dbits++;

			if (token == FIELD_END && (i+1) == fbits) {
				fprintf(out,"\t\t}\n");
				fprintf(out,"\t}\n");
				fprintf(out,"/* CSR bits defined: %d */\n", dbits);
				fprintf(out,"    }\n");
				fclose(out);
				out = NULL;
			} else {
				fprintf(out,"\t\t},\n");
			}
		}
		fbits = 0;
	} else {
		exit(0);
	}
	goto next;
#endif
}

