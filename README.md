# xquotes_history
С++ header-only библиотека для хранения котировок

### Описание

Данная библиотека позволяет хранить котировки в сжатом формате и получать простой доступ к любой цене по метке времени.

### Особенности

* Библиотека позволяет сжимать котировки до *43 раз* при использовании только цен закрытия бара и до *13 раз* при использовании полноценного бара без объема (*в сравнении с csv файлами*)
* Библиотека использует упорядоченные по дням данные в бинарном виде, процессор не тратит время на парсинг текста
* Библиотека позволяет использовать удобный и быстрый способ получить любую цену по метке времени бара
* Бибиотека поддерживает стандарт C++11

### Структура хранилища котировок
![storage_structure](doc/storage_structure_600x415.png)
* Библиотека разбивает котировки по дням. Каждый торговый день записывается в отдельный подфайл
* Каждый подфайл одного дня содержит фиксированное количество минут. В одном дне *1440* минут
* Подфайлы могут содержать как весь набор цен одного бара *(open, low, high, close)* или *(open, low, high, close, volume)*, так и только одну цену, например *close*
* Каждый подфайл начинает котировки в *00:00:00* дня по *GMT*
* Подфайлы котировок содержат только котировки. Подфайл не содержит меток времени
* Подфайлы могут быть сжаты при помощи библиотеки *zstd* с применением специально подготовленного словаря
* Каждая котировка внутри подфайла хранится в *uint32_t* (занимает 4 байта) и получается путем умножения цены на множитель *100000*. Это позволяет хранить как 4-х, так и 5-ти значные котировки.
* В конец файла, хранящего подфайлы, записывается заголовок. В начале файла находится ссылка на заголовок (смещение в файле), которая занимает 4 байта.
* Заголовок содержит количество подфайлов, ключи подфайлов, размер подфайлов, ссылки на подфайлы и заметку. На каждую переменную отводится 4 байта.
* Ключ подфайла является номером дня с начала unix-времени. 

### Как начать использовать

* Для начала необходимо собрать библиотеку zstd. Если вы используете *Code::Blocks*, тогда в каталоге *code_blocks\build_zstd* вы найдете проект для сборки данной библиотеки. Не забудьте выбрать\настроить компилятор перед сборкой! Полученный файл *libzstd.a* положите в папку *lib*
* Укажите в проекте пути к файлам библиотек *xtime_cpp* и *banana-filesystem-cpp*

### Зависимости

Данная библиотека для работы использует следующие внешние библиотеки

* *Библиотека zstd (для работы с сжатыми файлами)* - [https://github.com/facebook/zstd](https://github.com/facebook/zstd)
* *Библиотека xtime (для работы с временными метками)* - [https://github.com/NewYaroslav/xtime_cpp.git](https://github.com/NewYaroslav/xtime_cpp.git)
* *Библиотека banana-filesystem-cpp (для работы с файлами)* - [https://github.com/NewYaroslav/banana-filesystem-cpp](https://github.com/NewYaroslav/banana-filesystem-cpp)


### Полезные ссылки
* Дракон-схемы - [https://drakonhub.com/ru](https://drakonhub.com/ru)
