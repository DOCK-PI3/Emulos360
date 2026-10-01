#include "avatar_studio.h"
#include "runtime_paths.h"
#include <QFile>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <qfloat16.h>
#include <algorithm>
#include <bit>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// Xenia's existing LZX decoder is linked into the native frontend as well.
int lzx_decompress(const void*, size_t, void*, size_t, uint32_t, void*, size_t);
extern "C" void xenia_log(const char* format, ...) {
    va_list args; va_start(args, format); std::vfprintf(stderr, format, args); va_end(args);
}
namespace av = xe::kernel::xam::emulos;
QString AvatarStudio::resourceRoot() const { return emulos::avatarResourceRoot(dataRoot_); }
namespace {
bool decode(av::AvatarBytes source, std::span<uint8_t> dest) {
    return lzx_decompress(source.data(), source.size(), dest.data(), dest.size(), 32768, nullptr, 0) == 0;
}
float half(uint16_t value) { return static_cast<float>(std::bit_cast<qfloat16>(value)); }
float snorm(uint32_t value, int bits) {
    const int32_t signedValue = static_cast<int32_t>(value << (32 - bits)) >> (32 - bits);
    return std::max(-1.0f, float(signedValue) / float((1 << (bits - 1)) - 1));
}
QColor color(uint32_t argb) { return QColor::fromRgba(argb); }
QString idText(const av::AvatarAssetId& id) { return QByteArray(reinterpret_cast<const char*>(id.data()), 16).toHex(); }
constexpr std::array<uint32_t, 19> categories = {4,8,16,32,64,128,256,512,1024,2048,0x100000,0x80000,0x200000,0x8000,0x2000,0x4000,0x10000,0x40000,0x20000};
constexpr std::array<size_t,9> featureOffsets={12,28,44,60,92,124,156,188,220};
struct Vertex {
    float position[3], normal[3], uv0[2], uv1[2], uv23[4], uv4[3], uv5[3];
};
QQuick3DGeometry* geometry(const av::AvatarModelBatch& batch) {
    auto* result = new QQuick3DGeometry;
    QByteArray vertices(qsizetype(batch.vertices.size() * sizeof(Vertex)), Qt::Uninitialized);
    QVector3D minimum(1e6f,1e6f,1e6f), maximum(-1e6f,-1e6f,-1e6f);
    size_t index = 0;
    for (const auto& input : batch.vertices) {
        Vertex v{};
        for (int axis = 0; axis < 3; ++axis) {
            v.position[axis] = input.position[axis] * 100;
            minimum[axis] = std::min(minimum[axis], v.position[axis]); maximum[axis] = std::max(maximum[axis], v.position[axis]);
        }
        v.normal[0] = snorm(input.normal & 0x7ff,11);
        v.normal[1] = snorm((input.normal >> 11) & 0x7ff,11);
        v.normal[2] = snorm(input.normal >> 22,10);
        for (int j=0;j<2;++j) { v.uv0[j]=half(input.uv[j]);v.uv1[j]=half(input.uv[2+j]);v.uv4[j]=half(input.uv[8+j]);v.uv5[j]=half(input.uv[10+j]); }
        for (int j=0;j<4;++j) v.uv23[j]=half(input.uv[4+j]);
        std::memcpy(vertices.data() + index++ * sizeof(Vertex), &v, sizeof(v));
    }
    result->setVertexData(vertices);
    result->setIndexData(QByteArray(reinterpret_cast<const char*>(batch.indices.data()), qsizetype(batch.indices.size()*2)));
    result->setStride(sizeof(Vertex)); result->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles); result->setBounds(minimum,maximum);
    using A=QQuick3DGeometry::Attribute;
    result->addAttribute(A::PositionSemantic,offsetof(Vertex,position),A::F32Type);
    result->addAttribute(A::NormalSemantic,offsetof(Vertex,normal),A::F32Type);
    result->addAttribute(A::TexCoord0Semantic,offsetof(Vertex,uv0),A::F32Type);
    result->addAttribute(A::TexCoord1Semantic,offsetof(Vertex,uv1),A::F32Type);
    result->addAttribute(A::ColorSemantic,offsetof(Vertex,uv23),A::F32Type);
    result->addAttribute(A::TangentSemantic,offsetof(Vertex,uv4),A::F32Type);
    result->addAttribute(A::BinormalSemantic,offsetof(Vertex,uv5),A::F32Type);
    result->addAttribute(A::IndexSemantic,0,A::U16Type);
    return result;
}
QQuick3DTextureData* texture(const av::AvatarModelTexture* input) {
    auto* result = new QQuick3DTextureData;
    if (!input || input->empty || input->pixels.empty()) {
        result->setSize(QSize(4,4));result->setFormat(QQuick3DTextureData::RGBA8);result->setTextureData(QByteArray(64,0));
    } else {
        const size_t layerSize = input->pixels.size() / input->layers;
        QByteArray pixels(reinterpret_cast<const char*>(input->pixels.data()),qsizetype(layerSize));
        for (qsizetype i=0;i+1<pixels.size();i+=2) std::swap(pixels[i],pixels[i+1]);
        const auto fmt=input->format&63;
        // QQuick3DTextureData does not upload compressed formats on every RHI.
        // Expand BC1/2/3 once when assembling the avatar, keeping rendering native.
        QByteArray rgba(qsizetype(input->width*input->height*4),0);
        const auto* source=reinterpret_cast<const uint8_t*>(pixels.constData());
        const size_t blockBytes=fmt==18?8:16;
        for(uint32_t by=0;by<(input->height+3)/4;++by)for(uint32_t bx=0;bx<(input->width+3)/4;++bx){
            const size_t offset=(by*((input->width+3)/4)+bx)*blockBytes;
            if(offset+blockBytes>size_t(pixels.size()))continue;
            const auto* block=source+offset;const auto* rgb=block+(fmt==18?0:8);
            const uint16_t c0=rgb[0]|(rgb[1]<<8),c1=rgb[2]|(rgb[3]<<8);
            uint8_t palette[4][4]{};
            for(int p=0;p<2;++p){const auto c=p?c1:c0;palette[p][0]=uint8_t(((c>>11)*255+15)/31);palette[p][1]=uint8_t((((c>>5)&63)*255+31)/63);palette[p][2]=uint8_t(((c&31)*255+15)/31);palette[p][3]=255;}
            for(int channel=0;channel<3;++channel){
                if(c0>c1||fmt!=18){palette[2][channel]=uint8_t((2*palette[0][channel]+palette[1][channel])/3);palette[3][channel]=uint8_t((palette[0][channel]+2*palette[1][channel])/3);}
                else palette[2][channel]=uint8_t((palette[0][channel]+palette[1][channel])/2);
            }
            palette[2][3]=255;palette[3][3]=(c0>c1||fmt!=18)?255:0;
            uint8_t alphas[8]{};uint64_t alphaIndices=0;
            if(fmt==20){alphas[0]=block[0];alphas[1]=block[1];for(int a=2;a<8;++a)alphas[a]=block[0]>block[1]?uint8_t(((8-a)*block[0]+(a-1)*block[1])/7):(a<6?uint8_t(((6-a)*block[0]+(a-1)*block[1])/5):uint8_t(a==6?0:255));for(int b=0;b<6;++b)alphaIndices|=uint64_t(block[2+b])<<(8*b);}
            const uint32_t indices=uint32_t(rgb[4])|(uint32_t(rgb[5])<<8)|(uint32_t(rgb[6])<<16)|(uint32_t(rgb[7])<<24);
            for(uint32_t y=0;y<4;++y)for(uint32_t x=0;x<4;++x){
                if(bx*4+x>=input->width||by*4+y>=input->height)continue;
                const uint32_t pixel=y*4+x;auto* dest=reinterpret_cast<uint8_t*>(rgba.data())+((by*4+y)*input->width+bx*4+x)*4;
                std::memcpy(dest,palette[(indices>>(pixel*2))&3],4);
                if(fmt==19)dest[3]=uint8_t(((block[pixel/2]>>((pixel%2)*4))&15)*17);
                if(fmt==20)dest[3]=alphas[(alphaIndices>>(pixel*3))&7];
            }
        }
        result->setFormat(QQuick3DTextureData::RGBA8);
        result->setSize(QSize(int(input->width),int(input->height)));result->setTextureData(rgba);
    }
    result->setHasTransparency(true);return result;
}
}

AvatarStudio::AvatarStudio(const QString& dataRoot,QObject* parent) : QObject(parent),dataRoot_(dataRoot) {
    animationTimer_.setInterval(33);connect(&animationTimer_,&QTimer::timeout,this,&AvatarStudio::animate);
}
int AvatarStudio::body() const { return av::AvatarBodyType(manifest_); }
QVariantList AvatarStudio::colors() const {
    QVariantList result;for(size_t i=0;i<9;++i)result.append(color(av::AvatarLoad32(manifest_,252+i*4)));return result;
}
QVariantList AvatarStudio::animations() const {
    QVariantList result;if(!catalog_||!skeleton_)return result;
    for(const auto& record:catalog_->records())if(record.component_mask==0x400000&&(record.body_mask&body())) {
        auto name=QString::fromStdU16String(record.names[record.names.size()>5?5:0]);
        if(name.isEmpty())for(const auto& localized:record.names)if(!localized.empty()){name=QString::fromStdU16String(localized);break;}
        if(name.isEmpty())name=tr("Animación %1").arg((int(record.id[4])<<8)|record.id[5]);
        result.append(QVariantMap{{"id",idText(record.id)},{"name",name}});
    }
    return result;
}
void AvatarStudio::setAnimation(const QString& text) {
    if(!catalog_||!skeleton_)return;const auto bytes=QByteArray::fromHex(text.toLatin1());if(bytes.size()!=16)return;
    av::AvatarAssetId id{};std::memcpy(id.data(),bytes.constData(),16);const auto* record=av::AvatarRecord(*catalog_,id);
    if(!record||record->component_mask!=0x400000||!(record->body_mask&body()))return;
    const auto asset=av::AvatarStructuredAsset::Parse(catalog_->Find(id));if(!asset)return;
    auto animation=av::AvatarAnimation::Parse(asset->Find(1));if(!animation||animation->joints!=skeleton_->joints.size())return;
    animation_=std::move(animation);animationId_=text;animationClock_.restart();animate();if(previewActive_)animationTimer_.start();emit changed();
}
bool AvatarStudio::loadCatalog() {
    if(catalog_)return true;
    QFile file(resourceRoot()+"/AvatarAssetPack.toc");
    if(!file.open(QIODevice::ReadOnly) || file.size()>64*1024*1024) {status_=tr("Falta importar el catálogo original de avatares.");emit changed();return false;}
    const auto bytes=file.readAll();catalog_=av::AvatarAssetCatalog::Parse({bytes.begin(),bytes.end()});
    if(!catalog_){status_=tr("El catálogo original está incompleto o no es compatible.");emit changed();return false;}
    QFile rig(resourceRoot()+"/avatar-skeleton.bin");
    if(rig.open(QIODevice::ReadOnly)&&rig.size()<8192){const auto bytes=rig.readAll();skeleton_=av::AvatarSkeleton::Parse({reinterpret_cast<const uint8_t*>(bytes.constData()),size_t(bytes.size())});}
    if(skeleton_) {
        for(const auto& record:catalog_->records())if(record.component_mask==0x400000&&record.id[4]==0&&record.id[5]==3){
            const auto asset=av::AvatarStructuredAsset::Parse(catalog_->Find(record.id));
            if(asset)animation_=av::AvatarAnimation::Parse(asset->Find(1));
            if(animation_&&animation_->joints!=skeleton_->joints.size())animation_.reset();
            if(animation_)animationId_=idText(record.id);
            break;
        }
    }
    animationClock_.start();return true;
}
void AvatarStudio::openProfile(const QString& xuid,const QByteArray& bytes) {
    profile_=xuid;ready_=false;dirty_=false;
    if(!loadCatalog()){emit opened();return;}
    if(bytes.size()==1000) std::memcpy(manifest_.data(),bytes.constData(),1000);
    else {manifest_=av::DefaultAvatarManifest(*catalog_,1);dirty_=true;}
    rebuild();refreshChoices();emit changed();emit opened();
}
bool AvatarStudio::rebuild() {
    std::string error;const auto scene=av::AvatarScene::Decode(*catalog_,manifest_,0x1fff,decode,&error);
    if(!scene){status_=tr("No se pudo ensamblar el avatar: %1").arg(QString::fromStdString(error));ready_=false;emit changed();return false;}
    animationTimer_.stop();meshes_.clear();const auto previous=objects_;objects_.clear();parts_.clear();
    for(const auto& component:scene->components)for(const auto& batch:component.model.batches){
        if(batch.vertices.empty()||batch.indices.empty())continue;
        auto* mesh=geometry(batch);mesh->setParent(this);objects_.append(mesh);
        meshes_.push_back({mesh,mesh->vertexData(),batch.vertices});
        QVariantMap part{{"geometry",QVariant::fromValue(mesh)},{"head",batch.shader==4},{"transparent",batch.shader==1||batch.shader==3}};
        const std::array<uint32_t,6> usages=batch.shader==4?std::array<uint32_t,6>{5,6,7,9,12,11}:std::array<uint32_t,6>{1,2,3,4,0,0};
        QVariantList textures,channels,customs;
        for(size_t slot=0;slot<6;++slot){
            const av::AvatarModelTexture* selected=nullptr;int uv=0;
            for(const auto& p:batch.parameters)if(p.type==1&&(p.usage==usages[slot]||(batch.shader==4&&((slot==2&&p.usage==8)||(slot==3&&p.usage==10))))){
                const auto idx=p.value[0]&0xffff;uv=int(p.value[0]>>16);if(idx<component.model.textures.size())selected=&component.model.textures[idx];break;
            }
            auto* data=texture(selected);data->setParent(this);objects_.append(data);textures.append(QVariant::fromValue(data));channels.append(uv);
        }
        for(size_t c=0;c<3;++c){
            QColor value=Qt::white;
            for(const auto& p:batch.parameters)if(p.type==3&&p.usage==22+c)value=QColor::fromRgbF(std::clamp(std::bit_cast<float>(p.value[0]),0.f,1.f),std::clamp(std::bit_cast<float>(p.value[1]),0.f,1.f),std::clamp(std::bit_cast<float>(p.value[2]),0.f,1.f));
            if(component.mask==2)value=color(scene->colors[0]);
            if(component.mask==4)value=color(scene->colors[1]);
            if(component.custom_colors[c]>>24)value=color(component.custom_colors[c]);
            customs.append(value);
        }
        part["textures"]=textures;part["uvChannels"]=channels;part["customColors"]=customs;parts_.append(part);
    }
    // Changing body (or opening another profile) must not retain an animation
    // excluded from that body's catalog, leaving the selector blank.
    if(skeleton_) {
        const auto available=animations();bool compatible=false;
        for(const auto& choice:available)if(choice.toMap()["id"].toString()==animationId_)compatible=true;
        if(!compatible){animation_.reset();animationId_.clear();if(!available.empty())setAnimation(available.first().toMap()["id"].toString());}
    }
    ready_=!parts_.empty();status_=tr("Avatar original · %1 componentes%2").arg(scene->components.size()).arg(animated()?tr(" · animación original"):QString{});emit sceneChanged();emit changed();
    if(animated()){animate();if(previewActive_)animationTimer_.start();}
    // Let QML release old model references before destroying GPU resources.
    for(auto* object:previous)object->deleteLater();return true;
}
void AvatarStudio::animate(){
    if(!animated()||animation_->frames.empty())return;
    const auto& animation=*animation_;const auto& rig=*skeleton_;
    const float frame=std::fmod(float(animationClock_.elapsed())*0.001f*animation.frames_per_second,float(animation.frames.size()));
    const auto first=size_t(frame),second=(first+1)%animation.frames.size();const float blend=frame-float(first);
    std::vector<QMatrix4x4> world(rig.joints.size()),skin(rig.joints.size());
    auto vector=[](const auto& a){return QVector3D(a[0],a[1],a[2]);};
    auto quaternion=[](const auto& a){return QQuaternion(a[3],a[0],a[1],a[2]);};
    for(size_t joint=0;joint<rig.joints.size();++joint){
        const auto& a=animation.frames[first][joint];const auto& b=animation.frames[second][joint];
        const auto& bindLocal=rig.joints[joint].local;
        QMatrix4x4 local;local.translate(vector(bindLocal.position)+vector(a.position)*(1-blend)+vector(b.position)*blend);
        local.rotate(quaternion(bindLocal.rotation)*QQuaternion::slerp(quaternion(a.rotation),quaternion(b.rotation),blend));local.scale(vector(bindLocal.scale)*(vector(a.scale)*(1-blend)+vector(b.scale)*blend));
        const auto parent=rig.joints[joint].parent;world[joint]=parent<0?local:world[size_t(parent)]*local;
        QMatrix4x4 bind;bind.translate(vector(rig.joints[joint].bind.position));bind.rotate(quaternion(rig.joints[joint].bind.rotation));
        skin[joint]=world[joint]*bind.inverted();
    }
    for(auto& mesh:meshes_){
        QByteArray vertices=mesh.original;
        for(size_t i=0;i<mesh.vertices.size();++i){
            const auto& original=mesh.vertices[i];Vertex v;std::memcpy(&v,mesh.original.constData()+i*sizeof(Vertex),sizeof(v));
            QVector3D position,normal;float total=0;
            for(int influence=0;influence<4;++influence){
                const int shift=24-8*influence;const auto joint=(original.joints>>shift)&255;const float weight=float((original.weights>>shift)&255)/255.f;
                if(!weight||joint>=skin.size())continue;
                position+=skin[joint].map(vector(original.position))*weight;normal+=skin[joint].mapVector(QVector3D(v.normal[0],v.normal[1],v.normal[2]))*weight;total+=weight;
            }
            if(total>0){position*=100.f/total;normal.normalize();for(int axis=0;axis<3;++axis){v.position[axis]=position[axis];v.normal[axis]=normal[axis];}}
            std::memcpy(vertices.data()+i*sizeof(Vertex),&v,sizeof(v));
        }
        mesh.geometry->setVertexData(vertices);mesh.geometry->setBounds(QVector3D(-150,-50,-150),QVector3D(150,250,150));
    }
}
void AvatarStudio::refreshChoices(){
    choices_.clear();if(!catalog_){emit choicesChanged();return;}
    const auto mask=categories[size_t(category_)];
    bool selectedAny=false;
    for(const auto& record:catalog_->records()){
        if(!(record.body_mask&body()) || (category_<10?(!(record.component_mask&mask)||record.component_mask>0x1fff):record.component_mask!=mask))continue;
        const size_t language=record.names.size()>5?5:0;const auto& name=record.names[language];
        if(category_==0 && QString::fromStdU16String(name).endsWith("(Hat)"))continue;
        bool selected=false;
        if(category_>=10){selected=av::AvatarManifestId(manifest_,featureOffsets[size_t(category_-10)])==record.id;}
        else for(size_t i=2;i<15;++i)if(av::AvatarManifestId(manifest_,288+i*32)==record.id)selected=true;
        selectedAny|=selected;
        choices_.append(QVariantMap{{"id",idText(record.id)},{"name",QString::fromStdU16String(name)},{"selected",selected}});
    }
    if((category_>=4&&category_<10)||category_>=16)choices_.prepend(QVariantMap{{"id",QString{}},{"name",tr("Ninguno")},{"selected",!selectedAny}});
    emit choicesChanged();
}
void AvatarStudio::setCategory(int category){if(category<0||category>=int(categories.size())||category==category_)return;category_=category;refreshChoices();}
void AvatarStudio::setBody(int body){if((body!=1&&body!=2)||!catalog_||body==this->body())return;const auto old=manifest_;manifest_=av::DefaultAvatarManifest(*catalog_,uint8_t(body));if(!rebuild()){const auto error=status_;manifest_=old;rebuild();status_=error;emit changed();return;}dirty_=true;refreshChoices();emit changed();}
void AvatarStudio::choose(const QString& text){
    if(!catalog_)return;
    if(text.isEmpty()&&((category_>=4&&category_<10)||category_>=16)){
        const auto old=manifest_;
        if(category_>=16)std::fill_n(manifest_.begin()+featureOffsets[size_t(category_-10)],32,0);
        else for(size_t i=2;i<15;++i){const size_t p=288+i*32;if(((manifest_[p+16]<<8)|manifest_[p+17])&categories[size_t(category_)])std::fill_n(manifest_.begin()+p,32,0);}
        if(!rebuild()){const auto error=status_;manifest_=old;rebuild();status_=error;}else dirty_=true;refreshChoices();emit changed();return;
    }
    const auto bytes=QByteArray::fromHex(text.toLatin1());if(bytes.size()!=16)return;
    av::AvatarAssetId id{};std::memcpy(id.data(),bytes.constData(),16);const auto* record=av::AvatarRecord(*catalog_,id);
    if(!record||!(record->body_mask&body())||(category_<10?(!(record->component_mask&categories[size_t(category_)])||record->component_mask>0x1fff):record->component_mask!=categories[size_t(category_)]))return;
    const auto old=manifest_;size_t offset=0;
    if(category_>=10){offset=featureOffsets[size_t(category_-10)];if(category_>=13)av::AvatarStore32(manifest_,offset+16,std::bit_cast<uint32_t>(1.0f));}
    else {
        for(size_t i=2;i<15;++i){const auto p=288+i*32;const uint32_t mask=(manifest_[p+16]<<8)|manifest_[p+17];if(mask&record->component_mask)std::fill_n(manifest_.begin()+p,32,0);if(!offset&&av::AvatarIdEmpty(av::AvatarManifestId(manifest_,p)))offset=p;}
        if(!offset)return;std::fill_n(manifest_.begin()+offset,32,0);manifest_[offset+16]=uint8_t(record->component_mask>>8);manifest_[offset+17]=uint8_t(record->component_mask);
    }
    av::AvatarSetManifestId(manifest_,offset,id);
    // A one-piece outfit may occupy both shirt and trousers. Replacing it with
    // separates restores any newly uncovered required slot from the catalog.
    if(category_<10){const auto defaults=av::DefaultAvatarManifest(*catalog_,uint8_t(body()));uint32_t covered=0;
        for(size_t i=2;i<15;++i){const auto p=288+i*32;covered|=(manifest_[p+16]<<8)|manifest_[p+17];}
        for(size_t required=0;required<4;++required)if(!(covered&(4u<<required)))for(size_t i=2;i<15;++i){const auto p=288+i*32;if(av::AvatarIdEmpty(av::AvatarManifestId(manifest_,p))){std::copy_n(defaults.begin()+352+required*32,32,manifest_.begin()+p);break;}}
    }
    if(!rebuild()){const auto error=status_;manifest_=old;rebuild();status_=error;emit changed();return;}dirty_=true;refreshChoices();emit changed();
}
void AvatarStudio::setColor(int index,const QColor& value){if(index<0||index>=9||!value.isValid()||!ready_)return;av::AvatarStore32(manifest_,252+size_t(index)*4,value.rgba());dirty_=true;rebuild();}
void AvatarStudio::save(){if(!ready_||profile_.isEmpty())return;emit saveRequested(profile_,QByteArray(reinterpret_cast<const char*>(manifest_.data()),1000));}
void AvatarStudio::saved(){dirty_=false;status_=tr("Avatar guardado en el perfil de Xbox 360.");emit changed();}
