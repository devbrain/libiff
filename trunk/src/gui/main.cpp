#include <QtGui>

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);

     QDirModel model;
     QTreeView tree;
     tree.setModel(&model);

     // Demonstrating look and feel features
     tree.setAnimated(false);
     tree.setIndentation(20);
     tree.setSortingEnabled(true);

     tree.setWindowTitle(QObject::tr("Dir View"));
     tree.resize(640, 480);
     tree.show();

     return app.exec();
 }
