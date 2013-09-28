#ifndef MSP_H
#define MSP_H

#include<stdexcept>
#include<stack>
#include<vector>

using std::string;

/* Wyjatki */

/* Wyjatek oznaczajacy blad zwiazany z plikiem (plik nie istnieje, zla sciezka */
class fs_exception : public std::exception {
    public:
        fs_exception(string text);
        virtual const char* what() const throw();
        virtual ~fs_exception() throw() {}
    private:
        string text;
};

/* Wyjatek oznaczajacy problem z zapisem/odczytem */
class io_exception : public std::exception {
    public:
        io_exception(string text);
        virtual const char* what() const throw();
        virtual ~io_exception() throw() {}
    private:
        string text;
};

class file_handler;
class dir_handler;
class symboliclink;

/* Glowna klasa reprezentujaca system plikow */
class Msp {
    public:
        /* Dostepen typy plikow */
        typedef enum {FILE, DIR, DIR_DESC, FIFO, LIFO, SYMLINK} file_type;
        /* Wskaznik na root */
        dir_handler* root;

        Msp(void);
        ~Msp(void);
        /* Tworzenie plikow i folderow */
        void create_file(const string& path);
        void create_fifo(const string& path);
        void create_lifo(const string& path);
        void create_dir (const string& path);
        void create_desc_dir(const string& path);

        /* Tworzenie dowiazan twardych i miekkich */
        void hardlink(const string& src, const string& target);
        void symlink(const string& src, const string& target);
        
        /* Operacje na plikach, folderach i dowiazaniach */
        void copy(const string& src, const string& target);
        void move(const string& src, const string& target);
        void remove(const std::string& path);

        /* Zwraca uchwyt do pliku */
        file_handler* operator[](const string& path);

        /* Pomocniczy copy */
        file_handler* copy_file(file_type type, const string text, const string& new_name);
    private:
        /* Pomocnicze ustawianie nazwy */
        void set_name(string name);
        dir_handler* find_dir(const string& path, dir_handler* last_dir, string& name);
};


/* Pomocnicza klasa, ktora trzyma licznik referencji */
class mystring {
    public:
        mystring() : ref_count(1), s() {};
        mystring(string str) : ref_count(1), s(str) {};
        mystring(mystring const& str) : ref_count(1), s(str.s) {};
        int ref_count;
        string s; 
};

/* Klasa reprezentujaca uchwyt do pliku */
class file_handler {
    public:
        mystring* text;
        std::vector<symboliclink*> symlinks;
        file_handler();
        file_handler(const string& name);
        const string& get_name() const;
        Msp::file_type get_type() const;
        /* Dzialania na mystring'u */
        void erase_text();
        void set_text(mystring* new_text);

        file_handler(file_handler const&); //DOPISANE
        virtual ~file_handler();
        virtual void read(string& buf, ssize_t bytes = 0);
        virtual void write(const string& buf);
        void set_name(const string& name);
    protected:
        string name;
        Msp::file_type type;

};

/* Klasa reprezentujaca obiekt /dev/null
   nie mozna z niego czytac, mozna zapisywac co sie chce, 
   tak jak w Linuxie */
class dev_null : public file_handler {
    public:
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
        dev_null(const string& name);
};

/* Klasa reprezentuajca obiekt /dev/zero
   mozna z niego czytac dowolna liczbe znakow '\0',
   nie mozna zapisywac */
class dev_zero : public file_handler {
    public:
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
        dev_zero(const string& name);
};

/* typedef coby sie ladnie pisalo */
typedef std::vector<file_handler*>::iterator files_it;

/* Klasa reprezentujaca uchwyt do folderu */
class dir_handler : public file_handler {
    public:
        /* Klasa reprezentujaca zwykly iterator, przechodzi po 
           wszystkich plikach i folderach, nie zaglebia sie do podfolderow */
        class iterator {
            public:
                iterator();
                iterator(iterator const& it);
                iterator& operator=(iterator const& it);
                iterator& operator++();
                file_handler& operator*();
                file_handler* operator->();
                bool operator==(iterator const& it);
                bool operator!=(iterator const&  it);
                /* Pozycja na wektorze */
                size_t current;
                /* Wskaznik na wektor (wiemy, ze brzydko :( ) */
                const std::vector<file_handler*>* pfiles;
        };
        /* Klasa reprezentujaca rekurencyjny iterator, przechodzi po
           wszystkich plikach, folderach i podfolderach */
        class rec_iterator : public iterator {
            public:
                rec_iterator();
                rec_iterator(rec_iterator const& it);
                rec_iterator& operator=(rec_iterator const& it);
                rec_iterator& operator++();
                bool operator==(rec_iterator const& it);
                bool operator!=(rec_iterator const& it);
                /* Stos iteratorow, sluzy do wracania do poprzenich folderow */
                std::stack<iterator> iterators;
        };
        /* Poczatki i konce iteratorow */
        iterator begin() const;
        const iterator end() const;
        rec_iterator rec_begin() const;
        const rec_iterator rec_end() const;
        
        virtual void read(string& buf, ssize_t bytes = 0);
        virtual void write(const string& buf);
     
        dir_handler(const string& name);
        dir_handler(dir_handler const& dir);
        ~dir_handler();
        void put(file_handler* file);
        file_handler * operator[](const string& path);
        /* Posortowana lista wszystkich plikow */
        std::vector<file_handler* > files;
};

/* Klasa reprezntujaca koljke fifo */
class fifo : public file_handler {
    public:
        fifo(const string& name);
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
};

/* Klasa reprezentujaca kolejke lifo */
class lifo : public file_handler {
    public:
        lifo(const string& name);
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
};

/* Klasa reprezentujaca folder z opisem,
   zachowuje sie jak zwykly folder z tym, ze
   mozna do niego pisac i z niego czytac */
class dir_desc : public dir_handler {
    public:
        dir_desc(const string& name);
        dir_desc(dir_desc const& dir);
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
};

/* Klasa reprezentujaca symboliczne dowiazanie */
class symboliclink : public file_handler {
    public:
        file_handler * linked_object;
        dir_handler * parent;
        symboliclink(const string& name, file_handler * file, const string& path, dir_handler * parent);
        void read(string& buf, ssize_t bytes = 0);
        void write(const string& buf);
        ~symboliclink();
};



#endif // MSP_H
