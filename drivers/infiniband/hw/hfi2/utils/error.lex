
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

#define ERRCAT_OK	"ERR_CATEGORY_OKAY"
#define ERRCAT_INFO	"ERR_CATEGORY_INFO"
#define ERRCAT_CORR	"ERR_CATEGORY_CORRECTABLE"
#define ERRCAT_TRANS	"ERR_CATEGORY_TRANSACTION"
#define ERRCAT_PROC	"ERR_CATEGORY_PROCESS"
#define ERRCAT_HFI	"ERR_CATEGORY_HFI"
#define ERRCAT_NODE	"ERR_CATEGORY_NODE"
#define ERRCAT_DEF	"ERR_CATEGORY_DEFAULT"	/* Software-only, this is to make sure all event bits get assigned into one of the others */

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
	char *p, dname[256], csrname[256], *temp = NULL;
	char fname[256], fcomment[4096], err_category[32];
	int i, fbits = 0, dbits = 0;
	int token;

#if DEBUG
	while (token=yylex()) {
		printf("token %d: %s\n", token, yytext);
	}

#else

	memset(err_category, '\0', 32);
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
		fprintf(out,"\t{\n");
		fprintf(out,"\t\t\"%s\",\n", dname);
		fprintf(out,"\t\t%s,\n",
			replace_str(csrname, "STS", "EN_HOST"));
		fprintf(out,"\t\t%s,\n",
			replace_str(csrname, "STS", "FIRST_HOST"));
		fprintf(out,"\t\t%s,\n", csrname);
		fprintf(out,"\t\t%s,\n",
			replace_str(csrname, "STS", "CLR"));
		fprintf(out,"\t\t%s,\n",
			replace_str(csrname, "STS", "FRC"));
		fprintf(out,"\t\t{\n");
	} else
	if (token == FIELD_NAME) {
		strcpy(fname, yytext);
	} else
	if (token == FIELD_BITS) {
		fbits = atoi(yytext);
		fcomment[0] = '\0';
	} else
	if (token == FIELD_COMMENT) {
		temp = strstr(&yytext[2], "ERR_CATEGORY");
		if (temp != (char *)NULL) {
			if (strncmp(temp, ERRCAT_OK, strlen(ERRCAT_OK)) == 0) {
				strcpy(err_category, ERRCAT_OK);
			} else if (strncmp(temp, ERRCAT_INFO, strlen(ERRCAT_INFO)) == 0) {
				strcpy(err_category, ERRCAT_INFO);
			} else if (strncmp(temp, ERRCAT_CORR, strlen(ERRCAT_CORR)) == 0) {
				strcpy(err_category, ERRCAT_CORR);
			} else if (strncmp(temp, ERRCAT_TRANS, strlen(ERRCAT_TRANS)) == 0) {
				strcpy(err_category, ERRCAT_TRANS);
			} else if (strncmp(temp, ERRCAT_PROC, strlen(ERRCAT_PROC)) == 0) {
				strcpy(err_category, ERRCAT_PROC);
			} else if (strncmp(temp, ERRCAT_HFI, strlen(ERRCAT_HFI)) == 0) {
				strcpy(err_category, ERRCAT_HFI);
			} else if (strncmp(temp, ERRCAT_NODE, strlen(ERRCAT_NODE)) == 0) {
				strcpy(err_category, ERRCAT_NODE);
			} else {
				printf("Strange error category = %s\n", temp);
			}
		} else {
			strcpy(err_category, ERRCAT_DEF);
		}
		strcat(fcomment, &yytext[2]);
	} else
	if (token == FIELD_TYPE || token == FIELD_END) {
		for (i = 0; i < fbits; i++) {
			fprintf(out,"\t\t\t{ /* bit %d */\n", dbits);
			fprintf(out,"\t\t\t\t\"%s\",\n", fname);
			fprintf(out,"\t\t\t\t\"%s\",\n", fcomment);
			fprintf(out,"\t\t\t\t%s,\n", err_category);
			dbits++;

			if (token == FIELD_END && (i+1) == fbits) {
				fprintf(out,"\t\t\t}\n");
				fprintf(out,"\t\t}\n");
				fprintf(out,"/* CSR bits defined: %d */\n", dbits);
				fprintf(out,"\t}\n");
				fclose(out);
				out = NULL;
			} else {
				fprintf(out,"\t\t\t},\n");
			}
		}
		fbits = 0;
	} else {
		exit(0);
	}
	goto next;
#endif
}
