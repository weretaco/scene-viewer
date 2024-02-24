cd shaders

shopt -s nullglob
shopt -s extglob

FILES=./!(*.spv)

for f in $FILES
do
   shaderName=$(echo $f | sed 's/\.\/\(.*\)\..*/\1/')
   shaderType=$(echo $f | sed 's/\.\/.*\.\(.*\)/\1/')
   fOut="$shaderName-$shaderType.spv"

   glslangValidator -V $f -o $fOut
done

cd ..