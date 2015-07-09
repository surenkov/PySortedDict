#include <Python.h>
#include <map>
#include <string>
#include <new>
#include <utility>

#define INCREF(o) (Py_INCREF(o), (o))
#define DECREF(o) (Py_DECREF(o), (o))

using namespace std;
using KeyPointer = PyObject *;
using ValuePointer = PyObject *;


class KeyComparer 
{
public:
    bool operator() (const KeyPointer a, const KeyPointer b) const
    {
        int res = PyObject_RichCompareBool(a, b, Py_LT);
        if (res == -1)
            PyErr_SetNone(PyExc_TypeError);
        return res;
    }
};

typedef struct {
    PyObject_HEAD
    map<KeyPointer, ValuePointer, KeyComparer> *dict;
} SortedDict;

typedef struct {
    PyObject_HEAD
    map<KeyPointer, ValuePointer, KeyComparer>::iterator it;
    map<KeyPointer, ValuePointer, KeyComparer>::iterator end;
} SortedDictIter;


extern "C" {
    PyMODINIT_FUNC PyInit_sorteddict(void);

    /* Dict methods */
    static void SortedDict_dealloc(SortedDict *);
    static PyObject *
        SortedDict_new(PyTypeObject *, PyObject *, PyObject *);
    static int SortedDict_traverse(SortedDict *, visitproc, void *);
    static int SortedDict_clear(SortedDict *);

    static int SortedDict_init(SortedDict *, PyObject *);
    static ssize_t SortedDict_length(SortedDict *);
    static int SortedDict_containsItem(SortedDict *, PyObject *);
    static PyObject *SortedDict_repr(SortedDict *);
    static PyObject *
        SortedDict_compare(SortedDict *, SortedDict *, int op);

    static PyObject *SortedDict_getItem(SortedDict *, PyObject *);
    static int SortedDict_setItem(SortedDict *, PyObject *, PyObject *);

    static PyObject *SortedDict_pop(SortedDict *, PyObject *);
    static PyObject *SortedDict_popItem(SortedDict *);
    static PyObject *
        SortedDict_getItemWithFallback(SortedDict *, PyObject *);
    static PyObject *SortedDict_copy(SortedDict *, PyObject *);
    static PyObject *SortedDict_clear_method(SortedDict *, PyObject *);
    static PyObject *SortedDict_keys(SortedDict *, PyObject *);
    static PyObject *SortedDict_values(SortedDict *, PyObject *);
    static PyObject *SortedDict_items(SortedDict *, PyObject *);
    static PyObject *SortedDict_update(SortedDict *, PyObject *);
    static PyObject *SortedDict_iter(SortedDict *self);

    /* Iterator methods */
    static void SortedDictIter_dealloc(SortedDictIter *);
    static PyObject *SortedDictIter_next(SortedDictIter *);
    static int 
        SortedDictIter_traverse(SortedDictIter *, visitproc, void *);
}
/* Helpers */
static int PopulateFromIterable(SortedDict *, PyObject *); 
static int PopulateFromMapping(SortedDict *, PyObject *);


static PyMethodDef SortedDict_methods[] = {
    {"get", (PyCFunction) SortedDict_getItemWithFallback, METH_VARARGS,
        "Returns mapped value with key or default value if specified."},

    {"keys", (PyCFunction) SortedDict_keys, METH_NOARGS, 
        "Returns list with key objects in sorted order."},

    {"values", (PyCFunction) SortedDict_values, METH_NOARGS,
        "Returns list with value objects in keys' sorted order."},

    {"items", (PyCFunction) SortedDict_items, METH_NOARGS,
        "Returns list wth (key, value) tuples in keys' sorted order."},

    {"clear", (PyCFunction) SortedDict_clear_method, METH_NOARGS,
        "Clears internal storage."},

    {"copy", (PyCFunction) SortedDict_copy, METH_NOARGS,
        "Creates a shallow copy of sorted dict."},

    {"update", (PyCFunction) SortedDict_update, METH_O,
        "Updates dictionary with other mapping's content."},

    {"pop", (PyCFunction) SortedDict_pop, METH_VARARGS,
        "If key is in the dictionary, removes it and returns its value, else returns default. "
        "If default is not given, and key is not in the dictionary, removes first item and returns its value. "
        "Otherwise, if dictionary is empty, a KeyError is raised." },

    {"popitem", (PyCFunction) SortedDict_popItem, METH_NOARGS,
        "If dictionary is not empty, removes first item and returns its value. Otherwise, raises a KeyError." },

    {NULL, NULL, 0, NULL}
};

static PyMappingMethods SortedDict_asMapping = {
    (lenfunc) SortedDict_length,
    (binaryfunc) SortedDict_getItem,
    (objobjargproc) SortedDict_setItem
};

static PySequenceMethods SortedDict_asSeq = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    (objobjproc) SortedDict_containsItem,
    NULL, NULL
};

static PyTypeObject SortedDictType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        "sorteddict.SortedDict",
    sizeof(SortedDict),
    0,
    (destructor)SortedDict_dealloc,
    0,
    0,
    0,
    0,
    (reprfunc)SortedDict_repr,
    0,
    &SortedDict_asSeq,
    &SortedDict_asMapping,
    0,
    0,
    (reprfunc)SortedDict_repr,
    0,
    0,
    0,
    Py_TPFLAGS_BASETYPE | 
        Py_TPFLAGS_DEFAULT | 
        Py_TPFLAGS_DICT_SUBCLASS |
        Py_TPFLAGS_HAVE_GC,
    "Mapping type, containing all items in ascending sorted key order",
    (traverseproc)SortedDict_traverse,
    (inquiry)SortedDict_clear,
    (richcmpfunc)SortedDict_compare,
    0,
    (getiterfunc)SortedDict_iter,
    0,
    SortedDict_methods,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (initproc)SortedDict_init,
    0,
    SortedDict_new,
    0,
};

static PyTypeObject SortedDictIterType = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"sortedddict.SortedDictIter",
	sizeof(SortedDictIter),			
	0,					
	(destructor)SortedDictIter_dealloc, 		
	0,	
	0,		
	0,			
	0,				
	0,					
	0,
	0,	
	0,		
	0,			
	0,				
	0,					
	PyObject_GenericGetAttr,		
	0,				
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
 	0,					
 	(traverseproc)SortedDictIter_traverse,	
 	0,			
	0,				
	0,					
	PyObject_SelfIter,			
	(iternextfunc)SortedDictIter_next,	
	0,			
	0,
};

static PyModuleDef sorteddictModule = {
    PyModuleDef_HEAD_INIT,
    "sorteddict",
    "Module with sorted dictionary class",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_sorteddict(void)
{
    if (PyType_Ready(&SortedDictType) < 0)
        return NULL;

    if (PyType_Ready(&SortedDictIterType) < 0)
        return NULL;

    PyObject *m = PyModule_Create(&sorteddictModule);
    if (!m) return NULL;

    Py_INCREF(&SortedDictType);
    PyModule_AddObject(m, "SortedDict", (PyObject *) &SortedDictType);
    return m;
}

static void 
SortedDict_dealloc(SortedDict *self)
{
    SortedDict_clear(self);
    delete self->dict;
    PyObject_GC_Del(self);
}

static PyObject *
SortedDict_new(PyTypeObject *type, PyObject *, PyObject *)
{
    SortedDict *self = PyObject_GC_New(SortedDict, type);
    if (self) 
        self->dict = new (nothrow) 
            map<KeyPointer, ValuePointer, KeyComparer>();
    if (self && !self->dict) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


static int 
SortedDict_traverse(SortedDict *self, visitproc visit, void *arg)
{
    for(auto kvp: *self->dict) {
        Py_VISIT(kvp.first);
        Py_VISIT(kvp.second);
    }  
    return 0;
}

static int
SortedDict_init(SortedDict *self, PyObject *args)
{
    PyObject *source = NULL;
    if (!PyArg_ParseTuple(args, "|O", &source))
        return -1;

    if (source) {
        if (PyMapping_Check(source)) 
            return PopulateFromMapping(self, source);
        else if (PyIter_Check(source))
            return PopulateFromIterable(self, source);
        else {
            PyErr_BadArgument();
            Py_DECREF(self);
            return -1;
        }
    }

    return 0;
}

static int
SortedDict_containsItem(SortedDict *self, PyObject *key)
{
    auto dict = self->dict;
    auto it = dict->find(key);
    return it != dict->end();
}

static int
SortedDict_clear(SortedDict *self)
{
    auto dict = self->dict;
    for (auto it = dict->begin(); it != dict->end(); it++) {
        Py_DECREF(it->first);
        Py_DECREF(it->second);
    }
    self->dict->clear();
    return 0;
}


static PyObject *
SortedDict_clear_method(SortedDict *self, PyObject *)
{
    SortedDict_clear(self);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
SortedDict_copy(SortedDict *self, PyObject *)
{
    SortedDict *other = 
        (SortedDict *) PyObject_CallObject((PyObject *) &SortedDictType,
                NULL);

    if (!other) return NULL;
    other->dict->insert(self->dict->begin(), self->dict->end());

    for (auto kvp: *other->dict) { 
        Py_INCREF(kvp.first);
        Py_INCREF(kvp.second);
    }
    return (PyObject *)other;
}

static PyObject *
SortedDict_getItem(SortedDict *self, PyObject *key)
{
    auto dict = self->dict;
    auto it = dict->find(key);
    if (it != dict->end()) {
        return INCREF(it->second);
    }

    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
}

static PyObject *
SortedDict_getItemWithFallback(SortedDict *self, PyObject *args)
{
    PyObject *key = NULL, *def = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &key, &def)) {
        PyErr_BadArgument();
        return NULL;
    }

    auto dict = self->dict;
    auto it = dict->find(key);
    if (it != dict->end()) 
        return INCREF(it->second);

    if (def != NULL)
        return INCREF(def);

    Py_RETURN_NONE;
}

static int 
SortedDict_setItem(SortedDict *self, PyObject *key, PyObject *value)
{
    auto dict = self->dict;
    auto it = dict->find(key);
    if (it != dict->end()) {
        Py_DECREF(it->first);
        Py_DECREF(it->second);
        dict->erase(it);
    }

    if (value) {
        dict->insert(make_pair(key, value));
        Py_INCREF(key);
        Py_INCREF(value);
    }
    else if (it == dict->end()) {
        PyErr_SetObject(PyExc_KeyError, key);
        return -1;
    }
    return 0;
}

static PyObject *
SortedDict_pop(SortedDict *self, PyObject *args)
{
    PyObject *key = NULL, *def = NULL;
    if (!PyArg_ParseTuple(args, "|OO", &key, &def)) {
        PyErr_BadArgument();
        return NULL;
    }

    auto dict = self->dict;
    auto it = dict->find(key);
    PyObject *res;

    if (it != dict->end()) {
        res = it->second;
        Py_DECREF(it->first);
        dict->erase(it);
        return res;
    } 

    if (def) 
        return INCREF(def);

    if (!dict->empty()) {
        it = dict->begin();
        res = it->second;
        Py_DECREF(it->first);
        dict->erase(it);
        return res;
    }

    if (key)
        PyErr_SetObject(PyExc_KeyError, key);
    else
        PyErr_SetNone(PyExc_KeyError);
    return NULL;
}

static PyObject *
SortedDict_popItem(SortedDict *self)
{
    auto dict = self->dict;
    if (dict->empty()) {
        PyErr_SetNone(PyExc_KeyError);
        return NULL;
    }

    auto it = dict->begin();
    PyObject *tuple = PyTuple_Pack(2, it->first, it->second);
    if (!tuple) {
        PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }

    dict->erase(it);
    return tuple;
}

static PyObject *
SortedDict_keys(SortedDict *self, PyObject *)
{
    auto dict = self->dict;
    PyObject *list = PyList_New(dict->size());
    if (!list) return NULL;

    size_t i = 0;
    for (auto it = dict->begin(); it != dict->end(); it++, i++)
        if (!PyList_SetItem(list, i, it->first))
            Py_INCREF(it->first);

    return list;
}

static PyObject
*SortedDict_values(SortedDict *self, PyObject *)
{
    auto dict = self->dict;
    PyObject *list = PyList_New(dict->size());
    if (!list) return NULL;

    size_t i = 0;
    for (auto it = dict->begin(); it != dict->end(); it++, i++)
        if (!PyList_SetItem(list, i, it->second))
            Py_INCREF(it->second);

    return list;
}

static PyObject *
SortedDict_items(SortedDict *self, PyObject *)
{
    auto dict = self->dict;
    PyObject *list = PyList_New(dict->size());
    if (!list) return NULL;

    size_t i = 0;
    for (auto it = dict->begin(); it != dict->end(); it++, i++) {
        PyObject *tuple = PyTuple_Pack(2, it->first, it->second);
        if (!tuple) {
            PyErr_SetNone(PyExc_RuntimeError);
            return NULL;
        }
        PyList_SetItem(list, i, tuple);
        Py_INCREF(it->first);
        Py_INCREF(it->second);
    }

    return list;
}

static PyObject *
SortedDict_update(SortedDict *self, PyObject *mapping)
{
    if (PyMapping_Check(mapping)) {
        PopulateFromMapping(self, mapping);
        return INCREF((PyObject *)self);
    }

    PyErr_BadArgument();
    return NULL;
}

static ssize_t
SortedDict_length(SortedDict *self)
{
    return self->dict->size();
}

static PyObject *
SortedDict_repr(SortedDict *self)
{
    auto dict = self->dict;
    if (dict->size() == 0)
        return PyUnicode_FromString("{}");

    string repr = "{";
    for (auto kvp: *dict) {
        char *keyRepr = PyUnicode_AsUTF8(PyObject_Repr(kvp.first));
        char *valRepr = PyUnicode_AsUTF8(PyObject_Repr(kvp.second));
        repr += keyRepr;
        repr += ": ";
        repr += valRepr;
        repr += ", ";
    }
    repr.erase(repr.end() - 2, repr.end());
    repr += "}";

    return PyUnicode_FromStringAndSize(repr.c_str(), repr.size());
}

static PyObject * 
SortedDict_compare(SortedDict *a, SortedDict *b, int op)
{
    if (!(op & (Py_EQ | Py_NE)))
        Py_RETURN_NOTIMPLEMENTED;

    if (!PyObject_TypeCheck(a, &SortedDictType) ||
            !PyObject_TypeCheck(b, &SortedDictType))
        Py_RETURN_NOTIMPLEMENTED;

    int eq = a->dict->size() == b->dict->size();    

    if (eq) {
        auto a_it = a->dict->begin();
        auto b_it = b->dict->begin();

        for(; 
                a_it != a->dict->end() && b_it != b->dict->end(); 
                a_it++, b_it++
            ) {
            int res = 
                PyObject_RichCompareBool(a_it->first, b_it->first, Py_EQ);
            if (res == -1) {
                if (!PyErr_Occurred())
                    PyErr_SetNone(PyExc_TypeError);
                return NULL;
            }
            if (!(eq &= res)) break;

            res = 
                PyObject_RichCompareBool(a_it->second, b_it->second, Py_EQ);
            if (res == -1) {
                if (!PyErr_Occurred())
                    PyErr_SetNone(PyExc_TypeError);
                return NULL;
            }

            if (!(eq &= res)) break;
            
        } 
    }

    if (op == Py_NE) 
        eq = !eq;

    if (eq)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
SortedDict_iter(SortedDict *self)
{
    SortedDictIter *obj =
       (SortedDictIter *) PyObject_GC_New(SortedDictIter, 
               &SortedDictIterType);
    if (!obj)
        return NULL;
    
    auto dict = self->dict;
    obj->it = dict->begin();
    obj->end = dict->end();

    Py_INCREF(obj->it->first);
    Py_INCREF(obj->it->second);
    return (PyObject *) obj;
}

/* Iterator methods 
 * ================ */
static void SortedDictIter_dealloc(SortedDictIter *self) 
{
    if (self->it != self->end) {
        Py_DECREF(self->it->first);
        Py_DECREF(self->it->second);
    }
    PyObject_GC_Del(self);
}

static PyObject *SortedDictIter_next(SortedDictIter *self)
{
    if (self->it == self->end)
        return NULL;

    auto it = self->it++;
    return INCREF(it->first);
}

static int 
SortedDictIter_traverse(SortedDictIter *self, visitproc visit, void *arg)
{
    Py_VISIT(self->it->first);
    Py_VISIT(self->it->second);
    return 0;
}


/* Helpers
 * ======= */
static PyObject *Iterable_GetIter(PyObject *iterable)
{
    PyObject *iter = PyObject_GetIter(iterable);
    if (!iter) {
        PyErr_BadArgument();
        return NULL;
    }

    return iter;
}

static int PopulateFromIterable(SortedDict *self, PyObject *iterable)
{
    PyObject *iter = NULL, *tuple = NULL,
             *keyObj = NULL, *valueObj = NULL;
    auto dict = self->dict;

    iter = Iterable_GetIter(iterable);
    if (!iter) goto error;

    while ((tuple = PyIter_Next(iter))) {
        keyObj = PyTuple_GetItem(tuple, 0);
        valueObj = PyTuple_GetItem(tuple, 1);

        if (!keyObj || !valueObj) goto error;

        auto it = dict->find(keyObj);
        if (it != dict->end()) {
            Py_DECREF(it->first);
            Py_DECREF(it->second);
            dict->erase(it);
        }

        dict->insert(make_pair(keyObj, valueObj));
        Py_INCREF(keyObj);
        Py_INCREF(valueObj);

        Py_DECREF(tuple);   
    }

    Py_DECREF(iter);
    return 0;

error:
    Py_XDECREF(iter);
    Py_XDECREF(tuple);
    Py_XDECREF(keyObj);
    Py_XDECREF(valueObj);
    return -1;
}

static int PopulateFromMapping(SortedDict *self, PyObject *mapping)
{
    PyObject *items = PyMapping_Items(mapping);
    int result = PopulateFromIterable(self, items);
    Py_CLEAR(items);
    return result;
}

