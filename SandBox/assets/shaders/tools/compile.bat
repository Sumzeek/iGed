:: .\slangc.exe ..\hello-world.slang -profile glsl_450 -target glsl -o ..\hello-world.glsl -entry computeMain
slangc.exe ../slang/Color.slang -profile glsl_450 -target glsl -o ../generated/glsl/Color.vert -entry vsMain
slangc.exe ../slang/Color.slang -profile glsl_450 -target glsl -o ../generated/glsl/Color.frag -entry psMain
slangc.exe ../slang/Texture.slang -profile glsl_450 -target glsl -o ../generated/glsl/Texture.vert -entry vsMain
slangc.exe ../slang/Texture.slang -profile glsl_450 -target glsl -o ../generated/glsl/Texture.frag -entry psMain
pause