/* * * * * * * * * * * * * * * * * * */
/*  Chest Archiver 1.0 source code   */
/*  (C) Simakov Alexander 11/07/2K   */
/* * * * * * * * * * * * * * * * * * */

#include "types.h"

#define MAX_BITS  14
#define INC_SIZE  256
#define END_FILE  257
#define TAB_RESET 258
#define MAX_CODE  1<<MAX_BITS
#define EMPTI MAX_CODE

#if MAX_BITS==14
 #define TAB_SIZE 18041
#endif
#if MAX_BITS==13
 #define TAB_SIZE 9029
#endif
#if MAX_BITS<=12
 #define TAB_SIZE 5021
#endif

typedef unsigned long int DWORD;
typedef unsigned int      WORD;
typedef unsigned char     BYTE;

FILE  *infile, *outfile;
BYTE  code_size;
WORD  next_code, sp, w_buf, r_buf, tmp_buf;
BYTE  free_bits;
WORD *code, *pref;
BYTE *root;
BYTE stack[MAX_CODE];

void put_code(WORD c) {
	if(free_bits >= code_size) {
		w_buf <<= code_size;
		w_buf |= c;
		if(free_bits == code_size) {
			fwrite(&w_buf, 2, 1, outfile);
			free_bits=16;
			return;
		}
	free_bits -= code_size;
	}
	else {
		w_buf <<= free_bits;
		w_buf |= c>>(code_size-free_bits);
		fwrite(&w_buf, 2, 1, outfile);
		w_buf = c;
		free_bits += 16 - code_size;
	}
}

void flush_buf() {
	if(free_bits != 16) {
		w_buf <<= free_bits;
		fwrite(&w_buf, 2, 1, outfile);
		free_bits = 16;
	}
}

WORD get_code() {
	WORD c;
	c = r_buf>>(32-code_size);
	r_buf <<= code_size;
	free_bits += code_size;

	if(free_bits >= 16) {
		tmp_buf = 0;
		fread(&tmp_buf, 2, 1, infile);
		tmp_buf <<= free_bits-16;
		r_buf |= tmp_buf;
		free_bits -= 16;
	}
	return c;
}

void fill_buf() {
	fread(&r_buf, 2, 1, infile);
	r_buf <<= 16;
}

DWORD file_size(FILE *fp) {
	DWORD save_pos, size_of_file;
	save_pos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	size_of_file = ftell(fp);
	fseek(fp, save_pos, SEEK_SET);
	return size_of_file;
}

WORD hash(WORD h_pref,BYTE h_root){
	register int index;
	register int offset;
	index=(h_root<<(code_size-8))^h_pref;
	if(index==0) offset=1;
	else offset=TAB_SIZE-index;
	while (1) {
		if(code[index]==EMPTI) return index;
		if(pref[index]==h_pref && root[index]==h_root) return index;
		index-=offset;
		if(index<0) index+=TAB_SIZE;
	}
}

void build_match(WORD c) {
	while(1) {
		stack[sp++]=root[c];
		if(pref[c]==EMPTI) break;
		c=pref[c];
	}
}

bool compress() {
	register WORD cur_pref;
	register BYTE cur_root;
	register WORD index;
	DWORD BYTEs_left;
	WORD  i;

	puts("Compressing...   ");
	code = malloc(TAB_SIZE*sizeof(WORD));
	pref = malloc(TAB_SIZE*sizeof(WORD));
	root = malloc(TAB_SIZE*sizeof(BYTE));
	if(code == NULL || pref == NULL || root == NULL) {
		printf("Fatal error allocating table space!\n");
		return false;
	}
	for(i=0; i<=TAB_SIZE-1; i++) code[i]=EMPTI;
	BYTEs_left=file_size(infile);
	cur_pref=fgetc(infile); BYTEs_left--;

	while(BYTEs_left--){
		cur_root=fgetc(infile);
		index=hash(cur_pref,cur_root);

		if(code[index]!=EMPTI) {
			cur_pref=code[index];
		}
		else {
			code[index]=next_code++;
			pref[index]=cur_pref;
			root[index]=cur_root;
			put_code(cur_pref);
			cur_pref=cur_root;

			if(next_code>>code_size) {
				if(next_code==MAX_CODE) {
					for(i=0;i<=TAB_SIZE-1;i++)
						code[i]=EMPTI;
					put_code(TAB_RESET);
					next_code=259;
					code_size=9;
				}
				else {
					put_code(INC_SIZE);
					code_size++;
				}
			}
		}
	}
	put_code(cur_pref);
	free(code); free(pref); free(root);
	put_code(END_FILE); flush_buf();
	return true;
}

bool decompress() {
	register WORD new_code;
	register WORD old_code;
	WORD  i;

	puts("Decompressing... ");
	pref=malloc(TAB_SIZE*sizeof(WORD));
	root=malloc(TAB_SIZE*sizeof(BYTE));
	if(pref==NULL || root==NULL) {
		printf("Fatal error allocating table space!\n");
		return false;
	}
	for(i=0;i<=255;i++) {
		root[i]=i;
		pref[i]=EMPTI;
	}
	fill_buf(); old_code=get_code();
	fputc(old_code,outfile);

	while((new_code=get_code())!=END_FILE) {
		if(new_code==TAB_RESET) {
			code_size=9;
			next_code=259;
			old_code=get_code();
			fputc(old_code,outfile);
			continue;
		}
		if(new_code==INC_SIZE) {
			code_size++;
			continue;
		}
		if(new_code<next_code) {
			build_match(new_code);
			for(i=1; i<=sp; i++) fputc(stack[sp-i],outfile);
		}
		else {
			build_match(old_code);
			for(i=1; i<=sp; i++) fputc(stack[sp-i],outfile);
			fputc(stack[sp-1],outfile);
		}
		pref[next_code]=old_code;
		root[next_code]=stack[sp-1];
		old_code=new_code;
		next_code++; sp=0;
	}
	free(pref); free(root);
	return true;
}

bool xz(bool action) {
	code_size=9;
	next_code=259;
	sp= w_buf = r_buf = tmp_buf = 0;
	free_bits=16;
	char *ng = malloc(MAX_PATH), *db = malloc(MAX_PATH);

	sprintf(ng,"%s/lzw", config_path);
	sprintf(db,"%s/db", config_path);
	bool res;
	if(action){	// сжатие
		infile  = fopen(db, "rb");
		outfile = fopen(ng, "wb");
		res = compress();
	}
	else {
		infile  = fopen(ng, "rb");
		outfile = fopen(db, "wb");
		res = decompress();
	}
	free(db); free(ng);
	fclose(infile); fclose(outfile);
	return res;
}
