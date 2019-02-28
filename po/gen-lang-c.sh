#!/bin/sh

### sh gen-lang-c.sh

GEN_FILE="lang.c"


echo "#include <base.h>" > $GEN_FILE
echo "" >> $GEN_FILE
echo "" >> $GEN_FILE

echo "typedef struct _Langs Langs;

struct _Langs
{
	const char *lang_name;
	MsgIdStr *msgidstr;
	uint num;
};" >> $GEN_FILE

echo "" >> $GEN_FILE
echo "static Langs langs_n[] =" >> $GEN_FILE
echo "{" >> $GEN_FILE

for lang in *.po
do
	STR=`echo $lang | sed 's|\.po||'`
	echo "    { \"$STR\"," $STR"_msgidstr_n, G_N_ELEMENTS (" $STR"_msgidstr_n ) }," >> $GEN_FILE
done

echo "};" >> $GEN_FILE
echo "" >> $GEN_FILE

echo "" >> $GEN_FILE
echo "typedef struct _MsgIdStr MsgIdStr;

struct _MsgIdStr
{
	const char *msgid;
	const char *msgstr;
};" >> $GEN_FILE


for file in *.po
do

LANG=`echo $file | sed 's|\.po||'`

echo "" >> $GEN_FILE
echo "static MsgIdStr" $LANG"_msgidstr_n[] =" >> $GEN_FILE
echo "{" >> $GEN_FILE

	MSG=`grep -E "(msgid|msgstr)" $file | grep -v "#"`
	STR=`echo $MSG | sed 's|msgid | },\n    { |g;s|msgstr |, |g'`
	echo "$STR }" >> $GEN_FILE

echo "};" >> $GEN_FILE
echo "" >> $GEN_FILE

done

echo "" >> $GEN_FILE
