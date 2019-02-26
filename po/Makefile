obj_locale = $(shell echo $(LANG) | cut -f 1 -d '.')

all: msgfmt

msgfmt:
	@echo
	@echo "Localization ( msgfmt ):"
	@for language in *.po; do \
		lang=`basename $$language | cut -f 1 -d '.'`; \
		mkdir -pv ../$(builddir)/locale/$$lang/LC_MESSAGES/; \
		msgfmt -v $$language -o ../$(builddir)/locale/$$lang/LC_MESSAGES/$(program).mo; \
	done

genpot:
	@xgettext ../src/*.c --language=C --keyword=N_ --keyword=_ --escape --sort-output --from-code=UTF-8 \
	--package-name=$(program) --package-version=$(version) -o $(program).pot
	@sed 's|charset=CHARSET|charset=UTF-8|g' -i $(program).pot

mergeinit: genpot
	@echo "Localization ( msginit or msgmerge ):"
	@for lang in $(obj_locale); do \
		echo "Language	" $$lang; \
		if [ ! -f $$lang.po ]; then msginit -i $(program).pot --locale=$$lang -o $$lang.po --no-translator; \
		else msgmerge --update $$lang.po $(program).pot; fi \
	done

msgmerge: genpot
	@for lang in *.po; do \
		echo $$lang; \
		msgmerge --update $$lang $(program).pot; \
	done

clean:
	rm -f *.pot *.po~

