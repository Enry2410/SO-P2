#!/bin/bash

# Crear el grupo ufvauditor
groupadd ufvauditor

# Crear usuarios para cada SUxxx y añadirlos al grupo ufvauditor
for i in {001..003}; do
  username="userSU$i"
  useradd -m -G ufvauditor $username
done

# Asignar permisos para cada usuario en su respectivo directorio
for i in {001..003}; do
  username="userSU$i"
  directory="files_data/SU$i"
  
  # Dar permisos de escritura y lectura (wr-) al usuario en su propio directorio
  setfacl -m u:$username:rw- $directory

  # Dar permisos de solo lectura (-r-) en los demás directorios SUxxx
  for j in {001..003}; do
    if [ $i -ne $j ]; then
      other_directory="files_data/SU$j"
      setfacl -m u:$username:r-- $other_directory
    fi
  done
done

# Dar permisos de lectura para el grupo ufvauditor en los ejecutables y otros ficheros en el directorio raíz
setfacl -m g:ufvauditor:r-- FileProcessor
setfacl -m g:ufvauditor:r-- Monitor
for file in *; do
  if [ -f "$file" ]; then
    setfacl -m g:ufvauditor:r-- "$file"
  fi
done

# Crear el grupo ufvauditores
groupadd ufvauditores

# Crear los usuarios userfp y usermonitor y añadirlos al grupo ufvauditores
useradd -m -G ufvauditores userfp
useradd -m -G ufvauditores usermonitor

# Dar permisos a userfp y usermonitor
# userfp: --X en FileProcessor y Monitor, wr- en todos los ficheros
setfacl -m u:userfp:--X FileProcessor
setfacl -m u:userfp:--X Monitor
find files_data/ -type f -exec setfacl -m u:userfp:rw- {} \;

# usermonitor: --X en Monitor y --- en todos los ficheros
setfacl -m u:usermonitor:--X Monitor
find files_data/ -type f -exec setfacl -m u:usermonitor:--- {} \;

# Configurar permisos por defecto para nuevos archivos en files_data/
setfacl -dm u:userfp:rw- files_data/
setfacl -dm u:usermonitor:--- files_data/
