# Build

```sh
export VERSION="1.0.0"
export VERSION_SUFFIX="-local"
export ASSEMBLY_VERSION="$VERSION.0"
dotnet build /property:Version=$VERSION$VERSION_SUFFIX /property:AssemblyVersion=$ASSEMBLY_VERSION --configuration Release
```

# Publish

```sh
export SOURCE="https://nuget.server/"
export KEY="NUGET_API_KEY"
dotnet nuget push ./bin/Release/DiffMatchPatch.$VERSION$VERSION_SUFFIX.nupkg --source $SOURCE --api-key $KEY
```
