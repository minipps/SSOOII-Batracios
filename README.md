# SSOOII-Batracios
PRÁCTICAS DE SISTEMAS OPERATIVOS II
=======================================
PRIMERA PRÁCTICA EVALUABLE
------------------------------
Batracios
-------------
1.  ### Enunciado.
    
    En la práctica que vais a realizar podréis aprender a usar los nuevos mecanismos IPC recientemente aprendidos. Se trata de simular mediante un programa la vida de unas ranas, inspirada en el famoso juego de consola clásico "Frogger".  
      
    Según se va ejecutando el programa, se ha de ver una imagen parecida a la siguiente:  
      
    
    ![visualización de la pantalla](http://avellano.usal.es/~ssooii/RANAS/ranas_unix.png)
    
      
      
    En la imagen pueden observarse las ranas, representadas por una "m" de color verde. Las ranas nacen de una de cuatro ranas madre de color verde oscuro y situadas en la parte inferior de la pantalla. El objetivo de sus vidas es atravesar un río. Para ello, pueden moverse hacia adelante, hacia la derecha o hacia la izquierda según su criterio, pero nunca hacia atrás. Deberán mantenerse dentro de la pantalla. Dos ranas no pueden ocupar la misma posición.  
      
    Cuando llegan las ranas a la orilla inferior del río, deben tener cuidado. Solo pueden saltar a las posiciones donde se encuentran unos troncos flotando sobre su superficie. Si saltan sobre el agua, se produce un error. Para aumentar la dificultad, los troncos están a la deriva moviéndose continuamente. Una rana puede desaparecer debido a que el tronco sobre el que se encuentra sale de la pantalla. Eso no constituye un error. Esa rana se ha "perdido".  
      
    En la parte superior de la pantalla aparecen tres contadores. El primero cuenta las ranas que han nacido. El segundo, las ranas que han alcanzado la orilla superior del río y se han puesto a salvo y el tercero, lleva cuenta de aquellas que se han perdido porque su tronco desapareció de la pantalla. Al final de la práctica, el número de ranas nacidas debe coincidir con el de ranas salvadas más el de ranas perdidas más el de ranas que permanecen en la pantalla.  
      
    El tiempo de la práctica se mide en "tics" de reloj. La equivalencia entre un tic y el tiempo real es configurable. Puede ser incluso cero. En ese caso la práctica irá a la máxima velocidad que le permita el ordenador donde se esté ejecutando.  
      
    En esta práctica usaréis una biblioteca de enlazado estático que se os proporcionará. El objetivo es doble: por un lado aprender a usar una de tales bibliotecas y por otro descargar parte de la rutina de programación de la práctica para que os podáis centrar en los problemas que de verdad importan en esta asignatura.  
      
    El programa constará de un único fichero fuente, `batracios.c`, cuya adecuada compilación producirá el ejecutable `batracios`. Respetad las mayúsculas/minúsculas de los nombres.  
      
    Para simplificar la realización de la práctica, se os proporciona una biblioteca estática de funciones (`libbatracios.a`) que debéis enlazar con vuestro módulo objeto para generar el ejecutable. Gracias a ella, algunas de las funciones necesarias para realizar la práctica no las tendréis que programar sino que bastará nada más con incluir la biblioteca cuando compiléis el programa. La línea de compilación del programa podría ser:
    
    gcc batracios.c libbatracios.a -o batracios -lm
    
    Disponéis, además, de un fichero de cabeceras, `batracios.h`, donde se encuentran definidas, entre otras cosas, las macros que usa la biblioteca y las cabeceras de las funciones que ofrece.  
      
    El proceso inicial se encargará de preparar todas las variables y recursos IPC de la aplicación y registrar manejadoras para las señales que necesite. Este proceso, además, debe tomar e interpretar los argumentos de la línea de órdenes y llamar a la función `BATR_inicio` con los parámetros adecuados. El proceso será responsable de crear los procesos adicionales necesarios, salvo los procesos que representan a las ranitas, que son hijos del proceso que maneja a la rana madre correspondiente. En ningún caso, puede la práctica mantener en ejecución simultánea más de 25 procesos. Las cuatro ranas madres procreadoras y cada una de la pequeñas ranitas que nazcan, serán representadas, por lo tanto, mediante un proceso. También es responsabilidad del primer proceso el controlar que, si se pulsa CTRL+C, la práctica acaba, no dejando procesos en ejecución ni recursos IPCs sin borrar. La práctica devolverá 0 en caso de ejecución satisfactoria o un número mayor que cero, en caso de detectarse un error.  
      
    La práctica se invocará especificando dos parámetros obligatorios desde la línea de órdenes. Si no se introducen argumentos, se imprimirá un mensaje con la forma de uso del programa por el canal de error estándar. El primer argumento será un número entero comprendido entre 0 y 1000 que indicará la equivalencia en ms de tiempo real de un tic de reloj. O dicho de otro modo, la "lentitud" con que funcionará la práctica. El segundo argumento es la media de tics de reloj que necesita una rana madre descansar entre dos partos. Es un número entero estrictamente mayor que 0. Si el primer parámetro es 1 o mayor, la práctica funcionará tanto más lenta cuanto mayor sea el parámetro y no deberá consumir CPU apreciablemente. El modo de conseguir la rapidez o lentitud lo realiza la propia biblioteca. Vosotros no tenéis más que pasar dicho argumento a la función de inicio. Si es 0, irá a la máxima velocidad, aunque el consumo de CPU sí será mayor. Por esta razón y para no penalizar en exceso la máquina compartida, no debéis dejar mucho tiempo ejecutando en el servidor la práctica a máxima velocidad.  
      
    El programa debe estar preparado para que, si el usuario pulsa las teclas CTRL+C desde el terminal, la ejecución del programa termine en ese momento y adecuadamente. Ni en una terminación como esta, ni en una normal, deben quedar procesos en ejecución ni mecanismos IPC sin haber sido borrados del sistema. Este es un aspecto muy importante y se penalizará bastante si la práctica no lo cumple.  
      
    Es probable que necesitéis semáforos o buzones para sincronizar adecuadamente la práctica. En ningún caso podréis usar en vuestras prácticas más de un array de semáforos, un buzón de paso de mensajes y una zona de memoria compartida. Se declarará un array de semáforos de tamaño adecuado a vuestros requerimientos, el primero de los cuales se reservará para el funcionamiento interno de la biblioteca. El resto, podéis usarlos libremente.  
      
    La biblioteca requiere memoria compartida. Debéis declarar una única zona de memoria compartida en vuestro programa. Los 2048 bytes primeros de dicha zona estarán reservados para la biblioteca. Si necesitáis memoria compartida, reservad más cantidad y usadla a partir del byte bimilésimo cuadragésimo noveno.  
      
    Las funciones proporcionadas por la biblioteca `libbatracios.a` son las que a continuación aparecen. De no indicarse nada, las funciones devuelven -1 en caso de error o, si siendo funciones booleanas, el resultado es falso. Las funciones devuelven 0 en caso contrario:
    
    *   `int BATR_inicio(int ret, int semAforos, int lTroncos[],int lAguas[],int dirs[], int tCriar, char *zona)`  
        El primer proceso, después de haber creado los mecanismos IPC que se necesiten y antes de haber tenido ningún hijo, debe llamar a esta función, indicando en `ret` la velocidad de presentación y en `tCriar` el tiempo medio entre partos de una rana madre (parámetros ambos de la línea de órdenes) y pasando además el identificador del conjunto de semáforos que se usará y el puntero a la zona de memoria compartida declarada para que la biblioteca pueda usarlos. El significado del resto de parámetros es el siguiente:
        *   `lTroncos`: array de siete enteros que contiene el valor de la longitud media de los troncos para cada fila. El índice cero del array se refiere a la fila superior de troncos.
        *   `lAguas`: igual que el parámetro anterior, pero referido a la longitud media del espacio entre troncos.
        *   `dirs`: lo mismo que los dos parámetros anteriores, pero en esta ocasión cada elemento puede valer `DERECHA`(0) o `IZQUIERDA`(1), indicando la dirección en que se moverán los troncos.
        *   Los valores del array de troncos y aguas deben ser todos estrictamente mayor que cero.
    *   `int BATR_avance_troncos(int fila)`  
        Hace avanzar una posición la fila de troncos en su dirección
    *   `void BATR_descansar_criar(void)`  
        Cada rana madre llama a esta función antes de dar a luz a una nueva ranita
    *   `int BATR_parto_ranas(int i,int *dx,int *dy)`  
        La rana madre llama a esta función para tener una ranita. El primer parámetro es el número de rana madre (de 0, rana de la izquierda a 3, rana de la derecha). En `dx` y `dy`, se devuelve la posición donde nace la rana. La rana madre, debe crear un nuevo proceso (o esperarse si ahora mismo hay el máximo) para que se haga cargo de la nueva rana
    *   `int BATR_puedo_saltar(int x,int y,int direcciOn)`  
        Una rana situada en `x` e `y` pregunta con esta función si puede avanzar en la dirección (`DERECHA`, `IZQUIERDA` o `ARRIBA`). Devuelve 0 si la rana puede saltar
    *   `int BATR_explotar(int x,int y)`  
        (_A partir de la versión 0.3_). Hace que la rana situada en la posición que se le pasa, explote y desaparezca. La rana se suma a la cuenta de ranas perdidas. Se usa en la versión en que no se mueven los troncos para matar a las ranas que se quedan atrapadas
    *   `int BATR_avance_rana_ini(int x,int y)`, `int BATR_avance_rana(int *x,int *y,int direcciOn)` y `int BATR_avance_rana_fin(int x,int y)` Una vez la rana sabe que puede avanzar, llama a estas tres funciones. Los parámetros son de significado evidente. No obstante, fijaos en que la segunda función recibe la posición pasada por referencia, de modo que, una vez realizado el avance, las nuevas coordenadas aparecen en las variables pasadas. Esas mismas nuevas coordenadas, se pasan a la última función
    *   `int BATR_pausa(void)` e `int BATR_pausita(void)`  
        Estas dos funciones producen una pausa, sin consumo de CPU
    *   `int BATR_comprobar_estadIsticas(int r_nacidas, int r_salvadas, int r_perdidas)`  
        Antes de que la práctica termine, hay que comprobar que la suma de ranas nacidas tiene que ser igual al número de ranas salvadas más el número de ranas perdidas más el número de ranas que hay en la pantalla. Esta función realiza la comprobación y se le ha de pasar la cuenta que hayáis realizado vosotros
    *   `int BATR_fin(void)`  
        El padre, una vez sabe que ha acabado la práctica y antes de realizar limpieza de procesos y mecanismos IPC debe llamar a esta función.
    
      
      
    Notas acerca de las funciones:
    
    1.  Se puede establecer la longitud media de troncos y su separación de todas las filas salvo la del medio. Independientemente del valor espedificado, esta fila presenta una sucesión de troncos de tamaño dos separados también una distancia de dos caracteres.
    2.  La secuencia para que una rana se mueva consiste en llamar primero a `BATR_avance_rana_ini`, luego a `BATR_avance_rana`, ambas con la posición donde se encuentra la rana. Al retornar de `BATR_avance_rana`, en las variables de posición ya se encontrará la nueva posición de la rana. Hay que llamar a la función `BATR_pausa` y, finalmente, a `BATR_avance_rana_fin`, con la nueva posición de la rana.
    
    Estad atentos pues pueden ir saliendo versiones nuevas de la biblioteca para corregir errores o dotarla de nuevas funciones.  
      
    El guión que seguirá el proceso padre será el siguiente:
    
    1.  Tomará los datos de la línea de órdenes y los verificará.
    2.  Iniciará las variables, mecanismos IPC, manejadoras de señales y demás.
    3.  Llamará a la función `BATR_inicio`.
    4.  Creará los procesos que se harán cargo de las ranas madre.
    5.  Entrará en un bucle infinito del que solamente saldrá si se pulsa CTRL+C. En ese bucle, se encargará de mover una a una cada fila de troncos. Entre fila y fila, hará una llamada a la función BATR\_pausita
    6.  Cuando se pulse CTRL+C, se engargará de finalizar todo ordenadamente y comprobará las estadísticas con la función `BATR_comprobar_estadIsticas`
    
      
      
    Por su parte, los procesos encargados de manejar a las ranas madre, también están en un bucle infinito:
    
    1.  Llama.a la función `BATR_descansar_criar`
    2.  Si el número de procesos que hay es el máximo, tiene que esperar, sin consumo de CPU, a que haya un "hueco" para el nuevo proceso.
    3.  Tiene una rana, llamando a `BATR_parto_ranas` y crea un nuevo proceso para que se encargue de la rana recién nacida
    
      
      
    Finalmente, los procesos de las ranitas en su bucle infinito, hacen lo que se ha indicado más arriba hasta que desaparecen de la pantalla, bien por la parte de arriba (salvadas) o por un lateral (perdidas)  
      
    Observad que existe mucha sincronización que no se ha declarado explícitamente y debéis descubrir dónde y cómo realizarla. Os desaconsejamos el uso de señales para sincronizar. Una pista para saber dónde puede ser necesaria una sincronización son frases del estilo: "después de ocurrido esto, ha de pasar aquello" o "una vez todos los procesos han hecho tal cosa, se procede a tal otra".  
      
    Respecto a la sincronización interna de la biblioteca, se usa el semáforo reservado para conseguir atomicidad en la actualización de la pantalla y las verificaciones. Para que las sincronizaciones que de seguro deberéis hacer en vuestro código estén en sintonía con las de la biblioteca, debéis saber que sólo las funciones que actualizan valores sobre la pantalla están sincronizadas mediante el semáforo de la biblioteca.  
      
    En esta práctica no se podrán usar ficheros para nada, salvo que se indique expresamente. Las comunicaciones de PIDs o similares entre procesos, si hicieran falta, se harán mediante _mecanismos IPC_.  
      
    Siempre que en el enunciado o LPEs se diga que se puede usar `sleep()`, se refiere a la _llamada al sistema_, no a la orden de la línea de órdenes.  
      
    Los mecanismos IPC (semáforos, memoria compartida y paso de mensajes) son recursos muy limitados. Es por ello, que vuestra práctica sólo podrá usar un conjunto de semáforos, un buzón de paso de mensajes y una zona de memoria compartida como máximo. Además, si se produce cualquier error o se finaliza normalmente, los recursos creados han de ser eliminados. Una manera fácil de lograrlo es registrar la señal SIGINT para que lo haga y mandársela uno mismo si se produce un error.  
      
    
    #### Biblioteca de funciones `libbatracios.a`
    
    Con esta práctica se trata de que aprendáis a sincronizar y comunicar procesos en UNIX. Su objetivo no es la programación, aunque es inevitable que tengáis que programar. Es por ello que se os suministra una biblioteca estática de funciones ya programadas para tratar de que no debáis preocuparos por la presentación por pantalla, la gestión de estructuras de datos (colas, pilas, ...) , etc. También servirá para que se detecten de un modo automático errores que se produzcan en vuestro código. Para que vuestro programa funcione, necesitáis la propia biblioteca `libbatracios.a` y el fichero de cabeceras `batracios.h`. La biblioteca funciona con los códigos de VT100/xterm, por lo que debéis adecuar vuestros simuladores a este terminal. También se usa la codificación UTF-8, por lo que necesitáis un programa de terminal que sepa interpretarlos. Los terminales de Linux lo hacen por defecto, pero si usáis Windows, debéis aseguraros de que el programa tiene capacidad para interpretarlos y que esta capacidad está activada. Si no es así notaréis caracteres basura en la salida de modo que no se verá nada. Es, además, conveniente que pongáis el color de fondo de la pantalla a negro y su tamaño, al menos, a 80x25 caracteres.
    
    ##### Ficheros necesarios:
    
    *   `libbatracios.a`: [para Solaris](http://avellano.usal.es/~ssooii/RANAS/UNIX/SOLARIS/libbatracios.a) (ver 0.3), [para el LINUX de clase](http://avellano.usal.es/~ssooii/RANAS/UNIX/LINUX/libbatracios.a) (ver 0.3),
    *   `batracios.h`: [Para todos](http://avellano.usal.es/~ssooii/RANAS/UNIX/batracios.h) (ver 0.3).
    
      
      
    
    ##### Registro de versiones:
    
    *   0.1: primera versión
    *   0.2: corrección de una deriva en la coordenada x
    *   0.3:
        1.  mejora en las animaciones
        2.  pausa mayor en los errores, mostrando el lugar del error
        3.  Nueva función: BATR\_explotar
* * *
Enunciado, fotos y código de libbatracios.a: 

* © 2010, 2020 Ana Belén Gil González y Guillermo González Talaván

Solución del ejercicio:

* © 2020  AlbaCruz y Francisco Gallego
* * *
