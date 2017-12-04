#define _XOPEN_SOURCE 500
#include "abb.h"
#include "lista.h"
#include <stdlib.h>
#define _SVID_SOURCE
#include <string.h>
#define apilar(lista,dato) lista_insertar_primero(lista,dato)
#define desapilar(lista) lista_borrar_primero(lista)

struct abb {
	abb_t* izq;
	abb_t* der;
	char* clave;
	void* dato;
	abb_comparar_clave_t cmp;
	abb_destruir_dato_t destruir_dato;
};

abb_t* abb_crear(abb_comparar_clave_t cmp, abb_destruir_dato_t destruir_dato){
	if (!cmp) return NULL;
	abb_t* arbol = calloc(1,sizeof(abb_t));
	if (!arbol) return NULL;
	arbol->cmp = cmp;
	arbol->destruir_dato = destruir_dato;
	arbol->clave = NULL;
	arbol->dato = NULL;
	arbol->izq = NULL;
	arbol->der = NULL;
	return arbol;
}

//IN: recibe clave, dato y funciones para un nodo abb
//OUT: false si hay error de memoria
//Crea un nodo abb en la direccion dada
bool crear_abb_nodo(abb_t** arbol,const char *clave, void *dato, abb_comparar_clave_t cmp, abb_destruir_dato_t destruir_dato){
	*arbol = abb_crear(cmp,destruir_dato);
	if(!(*arbol)) return false;
	(*arbol)->clave = strdup(clave);
	if (!(*arbol)->clave) {
        abb_destruir(*arbol);
        return false;
    }
	(*arbol)->dato = dato;
	return true;	
}

bool abb_guardar(abb_t *arbol, const char *clave, void *dato) {
	if (!arbol) return false;
	if (arbol != NULL && !arbol->clave){
		arbol->clave = strdup(clave);
		arbol->dato = dato;
	}else{
		int cmp = arbol->cmp(arbol->clave,clave);
		if (cmp == 0 ) {
			if(arbol->destruir_dato) arbol->destruir_dato(arbol->dato);
			arbol->dato = dato;
			return true;
		} 
		if (cmp > 0){
			if (arbol->izq) return abb_guardar(arbol->izq,clave,dato);
			return crear_abb_nodo(&arbol->izq, clave, dato, arbol->cmp, arbol->destruir_dato);
		}
		if (arbol->der)	return abb_guardar(arbol->der,clave,dato);
		return crear_abb_nodo(&arbol->der, clave, dato, arbol->cmp, arbol->destruir_dato);
	}
	return true;
}

void *abb_obtener(const abb_t *arbol, const char *clave){
	if (!arbol || !arbol->clave) return NULL;
	int cmp = arbol->cmp(arbol->clave,clave);
	if (cmp == 0 )	return arbol->dato;
	if (cmp > 0){
		return abb_obtener(arbol->izq,clave);
	}
	return abb_obtener(arbol->der,clave);
}

bool abb_pertenece(const abb_t *arbol, const char *clave){
	if (arbol == NULL) return false;
	if (arbol->clave == NULL) return false;
	int cmp = arbol->cmp(arbol->clave,clave);
	if (cmp == 0 )	return true;
	if (cmp > 0){
		if (arbol->izq) return abb_pertenece(arbol->izq,clave);
		return false;
	}
	if (arbol->der)	return abb_pertenece(arbol->der,clave);
	return false;
}

size_t abb_cantidad(abb_t *arbol){
	if (!arbol) return 0;
	if (!arbol->clave) return 0;
	return (abb_cantidad(arbol->izq) + abb_cantidad(arbol->der) +1);
}

//busca el nodo mas chico o el mas grande, NULL para no buscar
abb_t* abb_min_max(abb_t* min, abb_t* max){
	if(min){
		if (!min->izq) return min;
		return abb_min_max(min->izq,NULL);
	}
	if(max){
		if (!max->der) return max;
		return abb_min_max(NULL,max->der);
	}
	return NULL;
}

//destruye un nodo "borrado"/"vacio" (existente pero sin clave)
void destruir_vacio(abb_t** arbol){
	if (*arbol == NULL) return;	
    if (!(*arbol)->clave){
		abb_destruir(*arbol);
		*arbol = NULL;
	}
}

//indica si un nodo no tiene hijos
bool abb_es_hoja(abb_t* arbol){
    if (!arbol) return false;
	if (!arbol->clave) return false;
    return (!arbol->izq && !arbol->der);
}

void destruir_hoja(abb_t** arbol){
    (*arbol)->dato = NULL;
    free((*arbol)->clave);
    (*arbol)->clave = NULL;
    destruir_vacio(&(*arbol)->izq);
    destruir_vacio(&(*arbol)->der);
}

//IN: recibe un nodo arbol, un hijo y se indica su sentido
//OUT: false si hay un error de memoria, true si se intercambiaron claves
//Elimina el nodo padre y sustituye su posicion en el arbol con la del nodo
//encontrado en la direccion dada cuya diferencia entre claves sea minima
bool abb_swap_borrar (abb_t** padre, abb_t** arbol, bool derecha){
    abb_t* swap;
    char* swap_clave;
    void *swap_dato;    
    if (!derecha){
        swap = abb_min_max(NULL,*arbol);
    }else{
        swap = abb_min_max(*arbol,NULL);
    }
    if (!swap->clave) return false;
    swap_clave = strdup(swap->clave);
    if (!swap_clave) return false;
    swap_dato = abb_borrar(*arbol, swap_clave);    
    destruir_vacio(&(*arbol));
    free((*padre)->clave);
    (*padre)->clave = swap_clave;
    (*padre)->dato = swap_dato;
    return true;
}

void *abb_borrar(abb_t *arbol, const char *clave){
	if (!arbol) return NULL;
    if(!arbol->clave) return NULL;
	void* dato; 
	int cmp = (arbol->cmp(clave, arbol->clave));
    if (cmp == 0){
		dato = arbol->dato;
        if (abb_es_hoja(arbol)){
            destruir_hoja(&arbol);
		}else{
            if (arbol->izq && arbol->izq->clave) {
                if(!abb_swap_borrar(&arbol, &arbol->izq,false)) return NULL;
            }
            else if (arbol->der && arbol->der->clave){
                if(!abb_swap_borrar(&arbol, &arbol->der, true)) return NULL;
            }
		}
	}
    else if (cmp < 0){
		dato = abb_borrar(arbol->izq, clave);
		destruir_vacio(&arbol->izq);
	}else{ //(cmp > 0)
		dato = abb_borrar(arbol->der, clave);
		destruir_vacio(&arbol->der);
	}
	return dato;
}

void abb_destruir(abb_t *arbol){
	if (!arbol) return;
	if(arbol->clave){
		abb_destruir(arbol->izq);
		abb_destruir(arbol->der);
		if (arbol->destruir_dato) arbol->destruir_dato(arbol->dato);
		free(arbol->clave);
		arbol->clave=NULL;
	}
	free(arbol);
}

//ITERADOR INTERNO
void _abb_in_order(abb_t * arbol, bool (*visitar)(const char *, void *, void *), void *extra, bool* seguir){
	if (!arbol || !visitar || !arbol->clave) return; 
	_abb_in_order(arbol->izq,visitar,extra,seguir);
	if(!visitar(arbol->clave,arbol->dato,extra)){
        *seguir = false;
        return;
    }
	_abb_in_order(arbol->der,visitar,extra,seguir);	
}

void abb_in_order(abb_t * arbol, bool (*visitar)(const char *, void *, void *), void *extra){
	if (!arbol || !visitar || !arbol->clave) return; 
    bool seguir = true;
	_abb_in_order(arbol->izq,visitar,extra,&seguir);
    if (!seguir) return;
	if(!visitar(arbol->clave,arbol->dato,extra)) return;
	_abb_in_order(arbol->der,visitar,extra,&seguir);	
}

//ITERADOR EXTERNO
void listar_izquierdos(const abb_t * arbol, lista_t* iter);

struct abb_iter{
	lista_t* pila; 
};

void listar_izquierdos(const abb_t * arbol, lista_t* iter){
	if (arbol == NULL) return;
	if(arbol->izq == NULL) return;
	apilar(iter,(void*)arbol->izq);
	listar_izquierdos(arbol->izq, iter);	
}

//pre: arbol creado
abb_iter_t *abb_iter_in_crear(const abb_t *arbol){
	abb_iter_t* abb_iter = calloc(1,sizeof(abb_iter_t));
	if (!abb_iter)  return NULL;
	lista_t* pila = lista_crear();
	if (!pila) {
        free(abb_iter);
        return NULL;
    }
	abb_iter->pila = pila;
	if(arbol->clave) {
		apilar(pila,(void*)arbol);
		listar_izquierdos(arbol, abb_iter->pila);
	}
	return abb_iter;
}

bool abb_iter_in_avanzar(abb_iter_t *iter){
	if(abb_iter_in_al_final(iter)) return false;
	abb_t* desapilado = (abb_t*)desapilar(iter->pila);
	if (!desapilado) return false; 
	if (desapilado->der) {
		apilar(iter->pila, desapilado->der);
		listar_izquierdos(desapilado->der,iter->pila);
	}
	return true;
}

const char *abb_iter_in_ver_actual(const abb_iter_t *iter){
	if (abb_iter_in_al_final(iter))  return NULL;
	return ((abb_t*)lista_ver_primero(iter->pila))->clave;	
}

bool abb_iter_in_al_final(const abb_iter_t *iter){
	return lista_esta_vacia(iter->pila);
}

void abb_iter_in_destruir(abb_iter_t* iter){
	lista_destruir(iter->pila,NULL);
	free(iter);
}
