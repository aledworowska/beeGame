#version 150 core

// Kolor ostateczny
out vec4 outColor;

// Dane wejsciowe
in vec4 Position;
in vec2 UV;
uniform sampler2D tex;
uniform int miniMap;
uniform int postprocessing;

void main()
{
    if(miniMap == 1)
    {
        outColor = texture(tex, UV);
    }
    else if(postprocessing == 1)
    {
        vec4 newColor = texture(tex, UV);
        float gray = (newColor.r + newColor.g + newColor.b)/3.0;

        outColor = vec4(gray,gray,gray,1.0);
    }
    else if(postprocessing == 2)
    {
        mat3 kernel ;
        kernel [0] = vec3 ( -1, -1, 1);
        kernel [1] = vec3 ( -1, 0, 1);
        kernel [2] = vec3 ( -1, 1, 1);
        vec2 texSize = vec2 ( textureSize ( tex, 0));
// Przetwarzanie splotowe
        vec3 result = vec3 (0.0);
        for ( int i = 0; i < 3; ++ i )
        {
            for ( int j = 0; j < 3; ++ j )
            {
                vec2 offsets = vec2 (i -3.0/2.0, j -3.0/2.0)/ texSize ;
                vec3 sampleColor = texture ( tex, UV + offsets ). rgb ;
                result += kernel [ i ][ j ] * sampleColor ;
            }
        }
        outColor = vec4 ( result, 0.0);
    }
}
