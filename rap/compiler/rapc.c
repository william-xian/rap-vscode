
#include <deed.h>
#include <rap_hash.h>
#include <rap_array.h>
#include <rapc.h>

#define VECTOR_SIZE 3 * 100
#define PATTERN_SIZE 20
#define  MSG_MAX_LEN 1024

char msg[MSG_MAX_LEN];

rap_pool_t *POOL;

enum range_type_t
{
	NONE,
	STRING,
	CLASS,
	DEED,
	FIELD,
	FUNCTION,
	INT,
	INT2,
	INT4,
	INT8,
};

typedef struct
{
	ushort start;
	ushort end;
} range_t;

def_a_(range_t)

typedef struct
{
	short class_no;
	int access;
	range_t name;
	range_t type;
	range_t val;
} field_t;

typedef struct
{
	short class_no;
	int access;
	range_t content;
	range_t name;
	rap_array_t *args;
	rap_array_t *returns;
	rap_array_t *expresses;
	/* data */
} function_t;

typedef struct
{
	range_t declare;
	range_t definition;
	range_t annocations;
	int modifier;
	range_t name;
	rap_array_t *typeParameters;
	rap_array_t *supers;
	rap_array_t *fields;
	rap_array_t *functions;
} class_t;

typedef struct
{
	char filename[256];
	ushort filesize;
	char *data;
	char *content;
	range_t package;
	a_range_t imports;
	a_range_t comments;
	a_range_t strings;
	rap_array_t classes;
} source_t;

typedef struct {
	int type;
	const char *names;
} word_t;

typedef struct {
	int type;
	const char *names;
} expr_t;
enum token_t{
	SET,CMP,BIT,ARITHMETIC,LGC,BRANCH,LOOP
};

word_t words[] = {
	{SET, "="},
	{SET, "+="},
	{SET, "-="},
	{SET, "*="},
	{SET, "/="},
	{SET, "|="},
	{SET, "&="},
	{SET, "^="},
	{CMP, ">"},
	{CMP, "<"},
	{CMP, "=="},
	{CMP, ">="},
	{CMP, "<="},
	{BIT, "&"},
	{BIT, "|"},
	{BIT, "^"},
	{BIT, "<<"},
	{BIT, "<<<"},
	{BIT, ">>"},
	{BIT, ">>>"},
	{ARITHMETIC, "+"},
	{ARITHMETIC, "++"},
	{ARITHMETIC, "-"},
	{ARITHMETIC, "*"},
	{ARITHMETIC, "/"},
	{LGC, "&&"},
	{LGC, "||"},
	{LGC, "!"},
	{BRANCH, "if"},
	{BRANCH, "else"},
	{BRANCH, "else if"},
	{BRANCH, "switch"},
	{BRANCH, "case"},
	{BRANCH, "default"},
	{BRANCH, "break"},
	{BRANCH, "continue"},
	{LOOP, "for"},
	{LOOP, "while"},
	{LOOP, "do...while"},
	{0, ""}
};

enum pattern_name
{
	COMMENT,
	CONST_STRING,
	CONST_CHAR,
	PACKAGE,
	IMPORT,
	CLASS_DEED,
	BACKET,
	COMMA,
	KEY_WORD
};

static char *patterns[PATTERN_SIZE] = {
	"/\\*(.|\n)*?\\*/",
	"\".*?\"",
	"'.*?'",
	"package .*;",
	"import .*;",
	"\\b(class|deed)\\b",
	"\\{(([^{}]*|(?R))*)\\}",
	"\\((([^()]*|(?R))*)\\)",
	"\\b(package|import|readable|writeable|callable|const|static|export|bool|byte|char|int2|int4|int8|float4|float8|string|enum|class|deed|this|super|null|any|if|else|switch|break|do|while|for|flow|goto|throw|throws|try|catch|finally|return)\\b",
	NULL};

static pcre *pcres[PATTERN_SIZE];

int errno;
const char *error;

void printRange(char *str, ushort start, ushort end)
{
	char buf[8196];
	strncpy(buf, &str[start], end - start);
	buf[end - start] = '\0';
	printf("%s\n", buf);
}

void source_replace_string(char *str, int len)
{
	char before = ' ', open = ' ', cur;
	for (int i = 0; i < len; i++)
	{
		cur = str[i];
		if (open != ' ')
		{
			if (str[i] == open)
			{
				if (before != '\\')
				{
					open = ' ';
				}
				else
				{
					str[i] = '-';
				}
			}
			else
			{
				str[i] = '_';
			}
		}
		else
		{
			if (str[i] == '"' || str[i] == '\'')
			{
				open = str[i];
			}
		}
		before = cur;
	}
}


void source_set_range(char *str, int start, int end, char c)
{
	for (int i = start; i < end; i++)
	{
		str[i] = c;
	}
}

int source_find_first_char_index(char *str, int start, int end)
{
	int i = start;
	while ((str[i] <= 32) && i < end)
		i++;
	return i;
}


int source_parse(source_t *src)
{
	log_d("启动") int rc;
	int ovector[3];
	int start = 0;
	
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[COMMENT], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			/* 删除注释 */
			range_t *val = rap_array_push(&src->comments);
			val->start = ovector[0];
			val->end = ovector[1];
			
			for (int i = ovector[0]; i < ovector[1]; i++)
			{
				src->content[i] = ' ';
			}
			start = ovector[1];
		}
		else
		{
			break;
		}
	}

	source_replace_string(src->content, src->filesize);
	start = 0;
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[CONST_STRING], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			range_t *val = rap_array_push(&src->strings);
			val->start = ovector[0];
			val->end = ovector[1];
			start = ovector[1];
		}
		else
		{
			break;
		}
	}
	start = 0;
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[CONST_CHAR], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			range_t *val = rap_array_push(&src->strings);
			val->start = ovector[0];
			val->end = ovector[1];
			start = ovector[1];
		}
		else
		{
			break;
		}
	}
	/** TODO check colsed */

	start = 0;
	rc = pcre_exec(pcres[PACKAGE], NULL, src->content, src->filesize, start, 0, ovector, 3);
	if (rc >= 0)
	{
		src->package.start = ovector[0];
		src->package.end = ovector[1];
		start = ovector[1];
		source_set_range(src->content, ovector[0], ovector[1], ' ');
		rc = pcre_exec(pcres[PACKAGE], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			snprintf(msg, MSG_MAX_LEN, "包名最多一个,%d,%d\n", ovector[0], ovector[1]);
			perror(msg);
			return 1;
		}
	}

	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[IMPORT], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{

			range_t *val = rap_array_push(&src->imports);
			val->start = ovector[0];
			val->end = ovector[1];

			source_set_range(src->content, ovector[0], ovector[1], ' ');
			start = ovector[1];
		}
		else
		{
			break;
		}
	}

	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[CLASS_DEED], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			rc = pcre_exec(pcres[BACKET], NULL, src->content, src->filesize, ovector[1], 0, ovector, 3);
			if (rc >= 0)
			{

				class_t *val = rap_array_push(&src->imports);
				val->declare.start = source_find_first_char_index(src->content, start, ovector[0]);
				val->declare.end = ovector[0] - 1;
				
				val->definition.start = ovector[0];
				val->definition.end = ovector[1];

				source_set_range(src->content, start, ovector[1], ' ');
				start = ovector[1];
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	return 0;
	return 0;
}

int source_load(source_t *src)
{
	FILE *fp = fopen(src->filename, "r");
	assert(fp != NULL);
	errno = fseek(fp, 0, SEEK_END);
	assert(errno == 0);
	int filesize = ftell(fp);
	errno = fseek(fp, 0, SEEK_SET);
	assert(errno == 0);
	src->filesize = filesize;
	src->data = malloc(filesize + 1);
	src->content = malloc(filesize + 1);
	fread(src->data, filesize, filesize, fp);
	src->data[filesize] = '\0';
	strncpy(src->content, src->data, filesize + 1);
	errno = fclose(fp);
	assert(errno == 0);
	return 0;
}

int source_init(source_t *src, char *filename)
{
	log_d("启动")
	range_t *pv;
	int i = 0;
	for (i = 0; patterns[i] != NULL; i++)
	{
		pcres[i] = pcre_compile(patterns[i], 0, &error, &errno, NULL);
		if (errno != 0)
		{
			log_e("pattern:%s, errno: %d, error: %s", patterns[i], errno, error);
		}
	}

	strcpy(src->filename, filename);
	src->filename[strlen(filename)] = '\0';
	source_load(src);

	rap_array_init(&src->imports, POOL, 8, sizeof(range_t));
	rap_array_init(&src->comments, POOL, 8, sizeof(range_t));
	rap_array_init(&src->strings, POOL, 8, sizeof(range_t));

	source_parse(src);

	for (i = 0; patterns[i] != NULL; i++)
	{
		pcre_free(pcres[i]);
	}

	//printf(">>>>>>>>>>> %s >>>>>>>>>>>\n%s\n<<<<<<<<<<< %s <<<<<<<<<<<\n", src->filename, src->content, src->filename);

	for (i = 0, pv = src->comments.elts; i < src->comments.nelts; i++,pv++)
	{	
		printRange(src->data, pv->start, pv->end);
	}
	for (i = 0, pv = src->strings.elts; i < src->strings.nelts; i++,pv++)
	{	
		printRange(src->data, pv->start, pv->end);
	}
	return 0;
}


void f() {
	rap_hash_keys_arrays_t ha;
	rap_hash_init_t classes;
	int v[3] = {1,2,3};
	rap_hash_key_t names = {};
	rap_hash_keys_array_init(&ha, 4);
	rap_hash_init(&classes,&names, 4);
	rap_str_t  key = rap_string("a");
	rap_hash_add_key(&ha, &key, &v[0], 0);
	rap_hash_find(classes.hash, rap_hash_key(key.data,key.len),key.data,key.len);
}

int main(int argc, char *argv[])
{
	log_d("启动")
	POOL = rap_create_pool(1024);
	source_t src;
	source_init(&src, argv[1]);
	rap_destroy_pool(POOL);
	return 0;
}


