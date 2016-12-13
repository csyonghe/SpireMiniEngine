pipeline StandardPipeline
{
    [Pinned]
    input world MeshVertex;
    
    [Pinned]
    input world ModelInstance;
    
    [Pinned]
    input world ViewUniform;
    
    [Pinned]
    input world MaterialUniform;
    
    [Pinned]
    input world LightData;

    world CoarseVertex;// : "glsl(vertex:projCoord)" using projCoord export standardExport;
    world Fragment;// : "glsl" export fragmentExport;
    
    require @CoarseVertex vec4 projCoord; 
    [Binding: "4"]
    extern @(CoarseVertex*, Fragment*) Uniform<LightData> LightDataBlock; 
    import(LightData->Fragment) uniformImport<T>()
    {
        return project(LightDataBlock);
    }
    import(LightData->CoarseVertex) uniformImport<T>()
    {
        return project(LightDataBlock);
    }

    [Binding: "2"]
    extern @(CoarseVertex*, Fragment*) Uniform<MaterialUniform> MaterialUniformBlock; 
    import(MaterialUniform->CoarseVertex) uniformImport<T>()
    {
        return project(MaterialUniformBlock);
    }
    import(MaterialUniform->Fragment) uniformImport<T>()
    {
        return project(MaterialUniformBlock);
    }

    [Binding: "1"]
    extern @(CoarseVertex*, Fragment*) Uniform<ViewUniform> ViewUniformBlock;
    import(ViewUniform->CoarseVertex) uniformImport<T>()
    {
        return project(ViewUniformBlock);
    }
    import(ViewUniform->Fragment) uniformImport<T>()
    {
        return project(ViewUniformBlock);
    }
    
    [Binding: "0"]
    extern @(CoarseVertex*, Fragment*) Uniform<ModelInstance> ModelInstanceBlock;
    import(ModelInstance->CoarseVertex) uniformImport<T>()
    {
        return project(ModelInstanceBlock);
    }
    import(ModelInstance->Fragment) uniformImport<T>()
    {
        return project(ModelInstanceBlock);
    }
        
    [VertexInput]
    extern @CoarseVertex MeshVertex vertAttribIn;
    import(MeshVertex->CoarseVertex) vertexImport()
    {
        return project(vertAttribIn);
    }
    
    extern @Fragment CoarseVertex CoarseVertexIn;
    import(CoarseVertex->Fragment) standardImport<T>()
        require trait IsTriviallyPassable(T)
    {
        return project(CoarseVertexIn);
    }
    
    stage vs : VertexShader
    {
        World: CoarseVertex;
        Position: projCoord;
    }
    
    stage fs : FragmentShader
    {
        World: Fragment;
    }
}


pipeline TessellationPipeline : StandardPipeline
{
    [Pinned]
    input world MeshVertex;
    
    [Pinned]
    [Binding: "0"]
    input world ModelInstance;
   
    [Pinned]
    [Binding: "1"]
    input world ViewUniform;
    
    [Pinned]
    [Binding: "2"]
    input world MaterialUniform;
    
    [Pinned]
    [Binding: "4"]
    input world LightData;

    [Binding: "4"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<LightData> LightDataBlock; 
    import(LightData->CoarseVertex) uniformImport<T>() { return project(LightDataBlock); }
    import(LightData->Fragment) uniformImport<T>() { return project(LightDataBlock); }
    import(LightData->ControlPoint) uniformImport<T>() { return project(LightDataBlock); }
    import(LightData->FineVertex) uniformImport<T>() { return project(LightDataBlock); }
    import(LightData->TessPatch) uniformImport<T>() { return project(LightDataBlock); }

    world CoarseVertex;
    world ControlPoint;
    world CornerPoint;
    world TessPatch;
    world FineVertex;
    world Fragment;
    
    require @FineVertex vec4 projCoord; 
    require @ControlPoint vec2 tessLevelInner;
    require @ControlPoint vec4 tessLevelOuter;
    [Binding: "2"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<MaterialUniform> instanceUniformBlock;
    
    import(MaterialUniform->CoarseVertex) uniformImport<T>() { return project(instanceUniformBlock); }
    import(MaterialUniform->Fragment) uniformImport<T>() { return project(instanceUniformBlock); }
    import(MaterialUniform->ControlPoint) uniformImport<T>() { return project(instanceUniformBlock); }
    import(MaterialUniform->FineVertex) uniformImport<T>() { return project(instanceUniformBlock); }
    import(MaterialUniform->TessPatch) uniformImport<T>() { return project(instanceUniformBlock); }
    
    [Binding: "1"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<ViewUniform> ViewUniformBlock;
    
    import(ViewUniform->CoarseVertex) uniformImport<T>() { return project(ViewUniformBlock); }
    import(ViewUniform->Fragment) uniformImport<T>() { return project(ViewUniformBlock); }
    import(ViewUniform->ControlPoint) uniformImport<T>() { return project(ViewUniformBlock); }
    import(ViewUniform->FineVertex) uniformImport<T>() { return project(ViewUniformBlock); }
    import(ViewUniform->TessPatch) uniformImport<T>() { return project(ViewUniformBlock); }
    
    [Binding: "0"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<ModelInstance> modelUniformBlock;
    
    import(ModelInstance->CoarseVertex) uniformImport() { return project(modelUniformBlock); }
    import(ModelInstance->ControlPoint) uniformImport() { return project(modelUniformBlock); }
    import(ModelInstance->FineVertex) uniformImport() { return project(modelUniformBlock); }
    import(ModelInstance->TessPatch) uniformImport() { return project(modelUniformBlock); }
    
    import(ModelInstance->Fragment) uniformImport() { return project(modelUniformBlock); }
    
    [VertexInput]
    extern @CoarseVertex MeshVertex vertAttribs;
    import(MeshVertex->CoarseVertex) vertexImport() { return project(vertAttribs); }
    
    // implicit import operator CoarseVertex->CornerPoint
    extern @CornerPoint CoarseVertex[] CoarseVertex_ControlPoint;
    [PerCornerIterator]
    extern @CornerPoint int sysLocalIterator;
    import (CoarseVertex->CornerPoint) standardImport<T>()
        require trait IsTriviallyPassable(T)
    {
        return project(CoarseVertex_ControlPoint[sysLocalIterator]);
    } 
    
    // implicit import operator FineVertex->Fragment
    extern @Fragment FineVertex tes_Fragment;
    import(FineVertex->Fragment) standardImport<T>()
        require trait IsTriviallyPassable(T)
    {
        return project(tes_Fragment);
    } 

    extern @ControlPoint CoarseVertex[] CoarseVertex_ControlPoint;
    extern @TessPatch CoarseVertex[] CoarseVertex_ControlPoint;
    [InvocationId]
    extern @ControlPoint int invocationId;
    import(CoarseVertex->ControlPoint) indexImport<T>(int id)
        require trait IsTriviallyPassable(T)
    {
        return project(CoarseVertex_ControlPoint[id]);
    }
    import(CoarseVertex->TessPatch) indexImport<T>(int id)
        require trait IsTriviallyPassable(T)
    {
        return project(CoarseVertex_ControlPoint[id]);
    }
    extern @FineVertex ControlPoint[] ControlPoint_tes;
    import(ControlPoint->FineVertex) indexImport<T>(int id)
        require trait IsTriviallyPassable(T)
    {
        return project(ControlPoint_tes[id]);
    }
    extern @FineVertex Patch<TessPatch> perPatch_tes;
    import (TessPatch->FineVertex) standardImport<T>()
        require trait IsTriviallyPassable(T)
    {
        return project(perPatch_tes);
    }
    
    extern @FineVertex Patch<CornerPoint[3]> perCorner_tes;
    [TessCoord]
    extern @FineVertex vec3 tessCoord;
    import(CornerPoint->FineVertex) standardImport<T>()
        require T operator + (T, T)
        require T operator * (T, float)
    {
        return project(perCorner_tes[0]) * tessCoord.x +
               project(perCorner_tes[1]) * tessCoord.y +
               project(perCorner_tes[2]) * tessCoord.z;
    }
      
    stage vs : VertexShader
    {
        VertexInput: vertAttribs;
        World: CoarseVertex;
    }

    stage tcs : HullShader
    {
        PatchWorld: TessPatch;
        ControlPointWorld: ControlPoint;
        CornerPointWorld: CornerPoint;
        ControlPointCount: 1;
        Domain: triangles;
        TessLevelOuter: tessLevelOuter;
        TessLevelInner: tessLevelInner;
    }
    
    stage tes : DomainShader
    {
        World : FineVertex;
        Position : projCoord;
        Domain: triangles;
    }
    
    stage fs : FragmentShader
    {
        World: Fragment;
        CoarseVertexInput: vertexOutputBlock;
    }
}