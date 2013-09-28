/*
    JNP1, Zadanie 6
    Wojciech Chmiel
*/

#include"msp.h"
#include<cstdio>
#include<cstddef>
#include<cstdlib>
#include<map>
#include<stdexcept>
#include<iostream>
#include<string>
#include<cctype>
#include<algorithm>
#include<cassert>

using std::string;
using std::vector;
using std::map;
using std::cout;
using std::endl;

typedef std::vector<file_handler*>::iterator files_it;
typedef dir_handler::iterator dir_it;
typedef dir_handler::rec_iterator dir_rec_it;

typedef vector<file_handler*>::iterator files_it;

/* Wyjatki */

fs_exception::fs_exception(string message) :
   text("fs_exception: " + message) {
   }

const char* fs_exception::what() const throw() {
   return text.c_str();
}

io_exception::io_exception(string message) :
   text("io_exception: " + message) {
   }

const char* io_exception::what() const throw() {
   return text.c_str();
}


/* odcina nazwe pierwszego folderu i daje reszte do path.
   przyklad /dev/aba -> return "dev", path = "aba", rest_length = 3
   /dev -> return dev, path = "", rest_length = 0 */
string get_first_and_split(string& path, size_t& rest_length) {
    size_t first  = path.find_first_of('/');
    string sought;
    if(first == string::npos) {
        // '/' nie znaleziozny, takze zwrocimy uchwyt i konczymy rekurencje
        sought = path;
        path.clear();
        rest_length = 0;
    }
    else {
        // '/' znazleziony, takze jeszcze przynajmniej jeden folder przed nami
        sought = path.substr(0, first);
        rest_length = path.size() - first - 1;
        string temp = path.substr(first + 1, rest_length);
        path = temp;
    }
    return sought;
}

/* odcina ostatniego '/' i zwraca nazwe za nim.
   Jesli byl tylko jeden '/', to zwraca samego '/' */
string remove_last_slash(string& path) {
    size_t pos = path.find_last_of('/');
    string res = path.substr(pos + 1);
    path.erase(pos);
    if(path.size() == 0)
        path.push_back('/');
    return res;
}

/* sprawdza czy podana sciezka nie zawiera nieprawidlowych znakow */
bool correct_pathname(const string& path) {
    if(path[0] != '/') return false;
    size_t first_wrong = path.find_first_not_of("abcdefghijklmnopqrstuvwxyz123456789./");
    return first_wrong == string::npos;
}

/* zmodyfikowane wyszukiwanie binarne(lower_bound) z cpluplus.com */
files_it lower_bound(files_it first, files_it last, const string& value) {
    files_it it;
    std::iterator_traits<files_it>::difference_type count, step;
    count = std::distance(first, last);
    while(count > 0) {
        it = first;
        step = count/2;
        advance(it, step);
        if((*it)->get_name() < value) {
            first = ++it;
            count -= step + 1;
        } else count = step;
    }
    return first;
}

/* konstruktor bezargumentowy do systemu plikow */
Msp::Msp(void) {
    root = new dir_handler("/");
    this->create_dir("/dev");
    file_handler* devnull = new dev_null("null");
    file_handler* devzero = new dev_zero("zero");
    dir_handler* dev = dynamic_cast<dir_handler*>((*(this))["/dev"]);
    dev->put(devnull);
    dev->put(devzero);
}

/* destruktor do systemu plikow */
Msp::~Msp(void) {
    delete root;
}

/* tworzenie pliku w systemie plikow, funkcja
   za argument bierze sciezke do pliku */
void Msp::create_file(const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    string name, rest_path(path);
    name = remove_last_slash(rest_path);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    file_handler* fh= new file_handler(name);
    parent->put(fh);
}   

/* tworzenie kolejki fifo w systemie plikow, funkcja
   za argument bierze sciezke do kolejki */
void Msp::create_fifo(const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    string name, rest_path(path);
    name = remove_last_slash(rest_path);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    file_handler* fh= new fifo(name);
    parent->put(fh);
}

/* tworzenie kolejki lifo w systemie plikow, funkcja
   za argument bierze sciezke do kolejki */
void Msp::create_lifo(const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    string name, rest_path(path);
    name = remove_last_slash(rest_path);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    file_handler* fh= new lifo(name);
    parent->put(fh);
}

/* tworzenie folderu w systemie plikow, funkcja
   za argument bierze sciezke do folderu */
void Msp::create_dir(const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    string name, rest_path(path);
    name = remove_last_slash(rest_path);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    file_handler* fh= new dir_handler(name);
    parent->put(fh);
}

/* tworzenie folderu z opisem w systemie plikow, funkcja
   za argument bierze sciezke do folderu */
void Msp::create_desc_dir(const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    string name, rest_path(path);
    name = remove_last_slash(rest_path);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    file_handler* fh= new dir_desc(name);
    parent->put(fh);
}

/* tworzenie twardego dowiazania */
void Msp::hardlink(const string& src, const string& target) {
    file_handler* fsource = (*this)[src];
    file_type type = fsource->get_type();
    if(type == DIR || type == DIR_DESC || type == SYMLINK)
        throw fs_exception("Can't make hard link to " + type);
    file_handler* ftarget;
    switch(type) {
        case LIFO: {
            create_lifo(target);
            break;
        } 
        case FIFO: {
            create_fifo(target);           
            break;
        }
        case Msp::FILE: {
            create_file(target);
            break;
        }
        default:
            cout << "Cos poszlo zle\n";
    }
    ftarget = (*this)[target];
    ftarget->erase_text();
    ftarget->set_text(fsource->text);
}

/* tworzenie dowiazania symbolicznego */
void Msp::symlink(const string& src, const string& target) {
    file_handler* fsource = (*this)[src];
    if(!correct_pathname(target))
        throw fs_exception("Incorrect path: " + target);
    string name, rest_target(target);
    name = remove_last_slash(rest_target);
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_target]);
    symboliclink* fh = new symboliclink(name, fsource, target, parent);
    parent->put(fh);
    fsource->symlinks.push_back(fh);
}

/* funkcja pomocnicza, zmieniajaca nazwe pliku */
void file_handler::set_name(const string& name1) {
    name = name1;
}

/* kopiowanie */
void Msp::copy(const string& src, const string& target) {
   file_handler* fh = (*this)[src];
   dir_handler* poss_dir = dynamic_cast<dir_handler*>(fh);
   file_handler* to_copy;
   if(poss_dir != NULL) {
       to_copy = new dir_handler(*poss_dir);
   }
   else {
       to_copy = new file_handler(*fh);
   }
   // wystarczy tam przeniesc
   string rest_target (target);
   string name_src = to_copy->get_name();
   // szukam katalogu docelowego
   string name_target = remove_last_slash(rest_target);
   dir_handler* dir_target = dynamic_cast<dir_handler*>((*this)[rest_target]);
   if(dir_target == NULL)
       throw fs_exception("Incorrect path: " + target);
    // sprawdzam czy plik docelowy juz istnieje
    size_t i=0;
    while(i<dir_target->files.size() && dir_target->files[i]->get_name() != name_target) i++;
    // czy istnieje juz cos o takiej nazwie
    if(i<dir_target->files.size()) {
        // sprawdzam czy to katalog
        if(dir_target->files[i]->get_type() == DIR || dir_target->files[i]->get_type() == DIR_DESC) {
            // bede przerzucal pliki do tego katalogu
            dir_target = dynamic_cast<dir_handler*>(dir_target->files[i]);
            if(dir_target == NULL)
                throw fs_exception("Incorrect path: " + target);
            name_target = name_src;
        }
        // to nie katalog
        else {
          // jest to plik tego samegu typu, moge go nadpisac
          if (dir_target->files[i]->get_type() == to_copy->get_type()) {
              files_it fit = lower_bound(dir_target->files.begin(), dir_target->files.end(), name_target);
              if((*fit)->get_name() == name_target) {
                file_handler * to_remove = (*fit);
                dir_target->files.erase(fit);
                delete to_remove;
              }
          }
          // jest to plik innego typu, rzucam wyjatek
          else throw fs_exception("Can't copy file " + target);
        }
    }
    // dodaje do nowego
    files_it fit = lower_bound(dir_target->files.begin(), dir_target->files.end(), name_target);
    dir_target->files.insert(fit, to_copy);
    symboliclink* sym = dynamic_cast<symboliclink*>(to_copy);
    if(sym != NULL)
        sym->parent = dir_target;
    // zmieniam mu nazwe
    to_copy->set_name(name_target); 
}

/* przenoszenie */
void Msp::move(const string& src, const string& target) {
    // znajduje file_handler do przenoszeonego pliku
    int found = target.find(src);
    // sprawdzam, czy nie probuje przeniesc pliku do wlasnego podkatalogu
    if (found == 0 && target != src)
        throw fs_exception("Can't move to " + target);
    string rest_src(src);
    string rest_target (target);
    // biore file_handler na przenoszony plik
    file_handler * source = (*this)[src];
    // szukam katalogu w ktorym znajduje sie src
    string name_src = remove_last_slash(rest_src);
    dir_handler* dir_src = dynamic_cast<dir_handler*>((*this)[rest_src]);
    if(dir_src == NULL)
        throw fs_exception("Incorrect path: " + src);        
    // szukam katalogu docelowego
    string name_target = remove_last_slash(rest_target);
    dir_handler* dir_target = dynamic_cast<dir_handler*>((*this)[rest_target]);
    if(dir_target == NULL)
        throw fs_exception("Incorrect path: " + target);
    // sprawdzam czy plik docelowy juz istnieje
    size_t i=0;
    while(i<dir_target->files.size() && dir_target->files[i]->get_name() != name_target) i++;
    // czy istnieje juz cos o takiej nazwie
    if(i<dir_target->files.size()) {
        // sprawdzam czy to katalog
        if(dir_target->files[i]->get_type() == DIR || dir_target->files[i]->get_type() == DIR_DESC) {
            // bede przerzucal pliki do tego katalogu
            dir_target = dynamic_cast<dir_handler*>(dir_target->files[i]);
            if(dir_target == NULL)
                throw fs_exception("Incorrect path: " + target);
            name_target = name_src;
        }
        // to nie katalog
        else {
          // jest to plik tego samegu typu, moge go nadpisac
          if (dir_target->files[i]->get_type() == source->get_type()) {
              files_it fit = lower_bound(dir_target->files.begin(), dir_target->files.end(), name_target);
              if((*fit)->get_name() == name_target) {
                file_handler * to_remove = (*fit);
                dir_target->files.erase(fit);
                delete to_remove;
              }
          }
          // jest to plik innego typu, rzucam wyjatek
          else throw fs_exception("Can't move file " + target);
        }
    }
    // usuwam obiekt z poprzedniego katalogu
    files_it fit = lower_bound(dir_src->files.begin(), dir_src->files.end(), name_src);
    dir_src->files.erase(fit);
    // dodaje do nowego
    fit = lower_bound(dir_target->files.begin(), dir_target->files.end(), name_target);
    dir_target->files.insert(fit, source);
    symboliclink* sym = dynamic_cast<symboliclink*>(source);
    if(sym != NULL)
        sym->parent = dir_target;
    // zmieniam mu nazwe
    source->set_name(name_target); 
}

/* usuwanie */
void Msp::remove(const string& path) {
    file_handler* to_remove = (*this)[path];
    string name, rest_path(path);
    name = remove_last_slash(rest_path);    
    dir_handler* parent = dynamic_cast<dir_handler*>((*this)[rest_path]);
    // path wskazuje na plik
    files_it fit = lower_bound(parent->files.begin(), parent->files.end(), name);
    if((*fit)->get_name() == name) {
        parent->files.erase(fit);
        delete to_remove;
    }
    else throw fs_exception("Incorrect path: " + path);
}

dir_handler::dir_handler(dir_handler const& dir) {
    this->name = dir.get_name();
    for(size_t i = 0; i < dir.files.size(); i++) {
        dir_handler* poss_dir = dynamic_cast<dir_handler*>(dir.files[i]);
        if(poss_dir == NULL) {
            // kopiujemy plik
            file_handler* fh = new file_handler(*(dir.files[i]));
            files.push_back(fh);
        } else {
            // kopiujemy folder
            dir_handler* dh = new dir_handler(*(poss_dir));
            files.push_back(dh);
        }
    }
}

/* Operator [] */
file_handler* Msp::operator[](const string& path) {
    if(!correct_pathname(path))
        throw fs_exception("Incorrect path: " + path);
    if(path.size() == 1)
        return root;
    string rest_path = path.substr(1, path.size() - 1);
    return (*root)[rest_path];
}

/* Konstruktory */

dir_desc::dir_desc(dir_desc const& dir)
    : dir_handler(dir) {
    this->text = new mystring(*(dir.text));
}

file_handler::file_handler()
    :name() {
    type = Msp::FILE;
    text = NULL;
}

file_handler::file_handler(const string& file_name) 
    : name(file_name) {
    text = new mystring();
    type = Msp::FILE;
}

file_handler::file_handler(file_handler const& file) 
    : name(file.name) {
    text = new mystring(*(file.text));
    type = file.get_type();
}

fifo::fifo(const string& file_name) 
    : file_handler(file_name) {
    type = Msp::FIFO;
}

lifo::lifo(const string& file_name) 
    : file_handler(file_name) {
    type = Msp::LIFO;
}

symboliclink::symboliclink(const string& file_name, file_handler * file, const string& path, dir_handler * parent1) 
    : file_handler(file_name) {
    type = Msp::SYMLINK;
    linked_object = file;
    parent = parent1;
}

void symboliclink::read(string& buf, ssize_t bytes) {
    linked_object->read(buf,bytes);
}

void symboliclink::write(const string& buf) {
    linked_object->write(buf);
}

/* Inne potrzebne funkcje */

const string& file_handler::get_name() const {
    return name;    
}

Msp::file_type file_handler::get_type() const {
    return type;
}

void file_handler::set_text(mystring* new_text) {
    text = new_text;
    text->ref_count++;
}

/* Destruktory */

file_handler::~file_handler() {
    if(text != NULL)
        erase_text();
    // usuwam wszystkie symlinki do siebie
    while (!symlinks.empty()){
        symboliclink * link = symlinks.back();
        symlinks.pop_back();
        files_it fit = lower_bound(link->parent->files.begin(), link->parent->files.end(), link->name);
        link->parent->files.erase(fit);
        delete link;
    }
}

dir_handler::~dir_handler() {
    for(size_t i = 0; i < files.size(); i++)
        delete files[i];
    // usuwam symlinki
    while (!symlinks.empty()){
        symboliclink * link = symlinks.back();
        symlinks.pop_back();
        files_it fit = lower_bound(link->parent->files.begin(), link->parent->files.end(), link->get_name());
        link->parent->files.erase(fit);
        delete link;
    }
}

symboliclink::~symboliclink() {
    size_t i = 0;
    while (i < linked_object->symlinks.size() && linked_object->symlinks[i] != this) i++;
    if (i < linked_object->symlinks.size())
        linked_object->symlinks.erase(linked_object->symlinks.begin()+i);
    // usuwam wszystkie symlinki do siebie
    while (!symlinks.empty()){
        symboliclink * link = symlinks.back();
        symlinks.pop_back();
        files_it fit = lower_bound(link->parent->files.begin(), link->parent->files.end(), link->name);
        link->parent->files.erase(fit);
        delete link;
    }
}

void file_handler::erase_text() {
    text->ref_count--;
    if(text->ref_count == 0)
        delete text;
    text = NULL;
}

dev_null::dev_null(const string& filename) 
    :file_handler() {
    name = filename;
}

dev_zero::dev_zero(const string& filename)
    :file_handler() {
    name = filename;
}

/* Funkcje read-write */

void file_handler::read(string& buf, ssize_t bytes) {
    if(bytes == 0)
        buf = text->s;
    else  buf = text->s.substr(0, std::min(bytes, (ssize_t)text->s.size()));
}

void file_handler::write(const string& buf) {
    text->s = buf;
}

void dev_null::read(string& buf, ssize_t bytes) {
    throw io_exception("Can't read from /dev/null");
}

void dev_null::write(const string& buf) 
{}

void dev_zero::write(const string& buf) {
    throw io_exception("Can't write to /dev/null");
}

void dev_zero::read(string& buf, ssize_t bytes) {
    if (bytes > 0){
        buf.clear();
        for(ssize_t i = 0; i < bytes; i++)
             buf.append("\0");
        }
}

void fifo::read(string& buf, ssize_t bytes) {
    if (bytes == 0) bytes = (ssize_t)text->s.size();
    bytes = std::min(bytes, (ssize_t)text->s.size());
    buf = text->s.substr(0, bytes);
    text->s.erase(0,bytes);   
}

void fifo::write(const string& buf) {
    text->s += buf;
}

void lifo::read(string& buf, ssize_t bytes) {   
    if (bytes == 0) bytes = (ssize_t)text->s.size();
    bytes = std::min(bytes, (ssize_t)text->s.size());
    buf = text->s.substr((ssize_t)text->s.size()-bytes, bytes);
    text->s.erase((ssize_t)text->s.size()-bytes,bytes);    
    reverse(buf.begin(), buf.end());
}

void lifo::write(const string& buf) {
    text->s += buf;
}

void dir_desc::read(string& buf, ssize_t bytes) {
    if (bytes == 0) bytes = (ssize_t)text->s.size();
    buf = text->s.substr(0, std::min(bytes, (ssize_t)text->s.size()));
}

void dir_desc::write(const string& buf) {
    text->s = buf;
}

void dir_handler::read(string& buf, ssize_t bytes) {
    throw io_exception("Can't read from directory " + (*this).get_name());
}

void dir_handler::write(const string& buf) {
    throw io_exception("Can't write to directory" + (*this).get_name());
}

dir_handler::dir_handler(const string& name) : file_handler() {
    this->name = name;
    type = Msp::DIR;
}

dir_desc::dir_desc(const string& file_name) 
    : dir_handler(file_name) {
    text = new mystring();
    type = Msp::DIR_DESC;
}

void dir_handler::put(file_handler* source) {
    string name = source->get_name();
    files_it fit = lower_bound(files.begin(), files.end(), name);
    if((fit != files.end()) && (files.size() > 0) && ((*fit)->get_name() == name)){ 
        // taki sam istnieje w tym katalogu
        throw fs_exception("File " + name + " already exists.");
    }
    files.insert(fit, source); 
}


file_handler* dir_handler::operator[](const string& path) {
    if(files.size() == 0)
        throw fs_exception("Incorrect path " + path);
    string sought, rest_path(path);
    size_t rest_length;
    sought = get_first_and_split(rest_path, rest_length);
    files_it fit = lower_bound(files.begin(), files.end(), sought);
    if((fit == files.end()) || ((*fit)->get_name() != sought))
        throw fs_exception("Incorrect path " + path);
    // znalezlismy cos
    if(rest_length == 0)
        return *fit;
    dir_handler* pdir = dynamic_cast<dir_handler*>(*fit);
    if(pdir == NULL) 
        throw fs_exception("Incorrect path " + path);
    // rekurencja
    return (*pdir)[rest_path];

}


/* Obsluga iteratorow */

dir_it::iterator()
: current(0), pfiles()
{}

dir_it::iterator(dir_it const& it)
: current(it.current), pfiles(it.pfiles)
{}

dir_it& dir_it::operator=(iterator const& it) {
   if(&it != this) {
        pfiles = it.pfiles;
        current = it.current;
   }
   return *this;
}

dir_it& dir_it::operator++() {
    current++;
    return *this;
}

file_handler& dir_it::operator*() {
    return  *((*pfiles)[current]);
}

file_handler* dir_it::operator->() {
    return (*pfiles)[current];
}

bool dir_it::operator==(dir_it const& it) {
    return ((current == it.current) && (*pfiles == *(it.pfiles)));
}

bool dir_it::operator!=(dir_it const& it) {
    return !(*this == it);
}

dir_rec_it::rec_iterator() 
    : dir_it(), iterators()
{}

dir_rec_it::rec_iterator(dir_rec_it const& it) 
    : dir_it(it), iterators(it.iterators)
{}

dir_rec_it& dir_rec_it::operator=(dir_rec_it const& it) {
   if(&it != this) {
        pfiles = it.pfiles;
        current = it.current;
        iterators = it.iterators;
   }
   return *this;

}

dir_rec_it& dir_rec_it::operator++() {
    while(!iterators.empty() && (current == pfiles->size() - 1)) {
        iterator it = iterators.top();
        iterators.pop();
        current = it.current + 1;
        pfiles = it.pfiles;
        if(current != pfiles->size() - 1)
            return *this;
    }
    dir_handler* poss_dir = dynamic_cast<dir_handler*>( (*pfiles)[current]);
    if((poss_dir == NULL) || (poss_dir->files.size() == 0))
        current++;
    else {
        iterator it;
        it.pfiles = pfiles;
        it.current = current;
        iterators.push(it);
        pfiles = &poss_dir->files;
        current = 0;
    }
    return *this;
}

bool dir_rec_it::operator==(dir_rec_it const& it) {
    if (!((current == it.current) && (*pfiles == *(it.pfiles)))) return false;
    std::stack<dir_it> s1(iterators), s2(it.iterators);
    if(s1.size() != s2.size()) return false;
    for(size_t i = 0; i < s1.size(); i++) {
        if(s1.top() != s2.top()) return false;
        s1.pop(); s2.pop();
    }
    return true;
}

bool dir_rec_it::operator!=(dir_rec_it const& it) {
    return !(*this == it);
}

dir_it dir_handler::begin() const {
    iterator it;
    it.pfiles = &files;
    return it;
}

const dir_it dir_handler::end() const {
    iterator it;
    it.pfiles = &files;
    it.current = files.size();
    return it;
}

dir_rec_it dir_handler::rec_begin() const {
    rec_iterator it;
    it.pfiles = &files;
    it.current = 0;
    return it;
}

const dir_rec_it dir_handler::rec_end() const {
    rec_iterator it;
    it.pfiles = &files;
    it.current = files.size();
    return it; 
}
