#include <QDialog>
#include <QStandardItemModel>

namespace LibGens {
    class Material;
    class ShaderLibrary;
}

class MaterialEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MaterialEditorDialog(QWidget *parent = nullptr);
    void setMaterial(LibGens::Material* material, LibGens::ShaderLibrary* shaderLibrary);

private slots:
    void onSaveClicked();
    void onAddTextureUnitClicked();
    void onDeleteTextureUnitClicked();
    // Add more slots as needed

private:
    LibGens::Material* m_material;
    LibGens::ShaderLibrary* m_shaderLibrary;
    QStandardItemModel* m_textureUnitsModel;
    // Add pointers to UI controls as needed
};
