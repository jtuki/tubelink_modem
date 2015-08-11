#ifndef _CFIFO_H
#define _CFIFO_H


#define fifoNull    ((void*)0)

typedef unsigned char fifoUint8;
typedef signed int fifoInt8;
typedef unsigned short fifoUint16;
typedef signed short fifoInt16;
typedef unsigned int fifoUint32;
typedef signed int fifoInt32;
typedef enum{ fifoFalse = 0, fifoTrue = !fifoFalse, fifoDisable = fifoFalse, fifoEnable = fifoTrue }fifoBool;



typedef struct{
    volatile fifoUint32 head;     /* input index */
    volatile fifoUint32 tail;     /* output index */
    volatile fifoUint32 capacity;   /* FIFO capacity */
}fifo_t;

/* all fifo will be the same format but array size different */
typedef struct{
    fifo_t tFifo;
    fifoUint8 au8Array[1];
}fifoBase_t;

#define FIFO_DEF(_fifo, _size)  \
typedef struct{\
    fifo_t tFifo;\
    fifoUint8 au8Array[_size];\
}_fifo;


#define fifo_Push( ptFifo, u8Data )     fifo_BasePush( (fifoBase_t*)ptFifo, u8Data )
#define fifo_Pop( ptFifo, pu8Data )     fifo_BasePop( (fifoBase_t*)ptFifo, pu8Data )

#define fifo_Increment__( _capacity, _idx )  \
    (( _idx + 1 ) % _capacity)

#define fifo_Init( ptFifo, _Size ) \
    (ptFifo)->tFifo.head = (ptFifo)->tFifo.tail = 0;\
    (ptFifo)->tFifo.capacity = _Size;

#define fifo_isEmpty( ptFifo ) \
    ( (ptFifo)->tFifo.head == (ptFifo)->tFifo.tail ? fifoTrue : fifoFalse )
    
#define fifo_isFull( ptFifo ) \
    ( (ptFifo)->tFifo.head == (((ptFifo)->tFifo.tail + 1) % (ptFifo)->tFifo.capacity) ? fifoTrue : fifoFalse )

static inline fifoBool fifo_BasePush(fifoBase_t *a_ptFifo, fifoUint8 a_u8Data){
    fifoBool bRet = fifoFalse;

    if( fifoFalse == fifo_isFull(a_ptFifo) ){
        a_ptFifo->au8Array[ a_ptFifo->tFifo.tail ] = a_u8Data;
        a_ptFifo->tFifo.tail = fifo_Increment__( a_ptFifo->tFifo.capacity, a_ptFifo->tFifo.tail );
        bRet = fifoTrue;
    }

    return bRet;
}

static inline fifoBool fifo_BasePop(fifoBase_t *a_ptFifo, fifoUint8 *a_pu8Data)
{
    fifoBool bRet = fifoFalse;
    if( fifoFalse == fifo_isEmpty(a_ptFifo) ){
        *a_pu8Data = a_ptFifo->au8Array[ a_ptFifo->tFifo.head ];
        a_ptFifo->tFifo.head = fifo_Increment__((a_ptFifo)->tFifo.capacity, (a_ptFifo)->tFifo.head);
        bRet = fifoTrue;
    }
    return bRet;
}

#define fifo_Clear( ptFifo ) \
    (ptFifo)->tFifo.head = (ptFifo)->tFifo.tail = 0;

#define fifo_Size( ptFifo ) \
    (( (ptFifo)->tFifo.tail >= (ptFifo)->tFifo.head ) ? (ptFifo)->tFifo.tail - (ptFifo)->tFifo.head : (ptFifo)->tFifo.capacity - (ptFifo)->tFifo.head + (ptFifo)->tFifo.tail)

#define fifo_Capacity( ptFifo )     ((ptFifo)->tFifo.capacity)
#endif
