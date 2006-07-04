/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.build.fpd;

import java.io.File;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

public class PlatformBuildFileGenerator {

    private String platformName;
    
    ///
    /// Mapping from modules identification to out put file name
    ///
    private Map<FpdModuleIdentification, String> outfiles;

    ///
    /// Mapping from FV name to its modules
    ///
    private Map<String, Set<FpdModuleIdentification>> fvs;

    ///
    /// Mapping from sequence number to FV names
    ///
    private Map<String, Set<String>> sequences;
    
    private boolean isUnified = true;
    
    private Project project;
    
    private String info = "DO NOT EDIT \n" 
        + "File auto-generated by build utility\n" 
        + "\n" 
        + "Abstract:\n"
        + "Auto-generated ANT build file for building of EFI Modules/Platforms\n";

    public PlatformBuildFileGenerator(Project project, Map<FpdModuleIdentification, String> outfiles, Map<String, Set<FpdModuleIdentification>> fvs, Map<String, Set<String>> sequences, boolean isUnified){
        this.project = project;
        this.outfiles = outfiles;
        this.fvs = fvs;
        this.sequences = sequences;
        this.isUnified = isUnified;
        this.platformName = project.getProperty("PLATFORM");
    }
    
    /**
      Generate build.out.xml file.
     
      @throws BuildException
                  build.out.xml XML document create error
    **/
    public void genBuildFile() throws BuildException {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder dombuilder = domfac.newDocumentBuilder();
            Document document = dombuilder.newDocument();
            Comment rootComment = document.createComment(info);
            //
            // create root element and its attributes
            //
            Element root = document.createElement("project");
            root.setAttribute("name", project.getProperty("PLATFORM"));
            root.setAttribute("default", "all");
            root.setAttribute("basedir", ".");
            
            //
            // element for External ANT tasks
            //
            root.appendChild(document.createComment("Apply external ANT tasks"));
            Element ele = document.createElement("taskdef");
            ele.setAttribute("resource", "GenBuild.tasks");
            root.appendChild(ele);

            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "frameworktasks.tasks");
            root.appendChild(ele);

            ele = document.createElement("property");
            ele.setAttribute("environment", "env");
            root.appendChild(ele);
            
            Set<String> sequenceKeys = sequences.keySet();
            Iterator sequenceIter = sequenceKeys.iterator();
            String dependsStr = "";
            while (sequenceIter.hasNext()) {
                String num = (String)sequenceIter.next();
                if (dependsStr.length() > 0) {
                    dependsStr += " , ";
                }
                dependsStr += "modules" + num + ", fvs" + num;
            }
            
            //
            // Default Target
            //
            root.appendChild(document.createComment("Default target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "all");
            ele.setAttribute("depends", dependsStr + ", userextensions");
            root.appendChild(ele);
            
            //
            // Modules and Fvs Target
            //
            sequenceIter = sequenceKeys.iterator();
            while (sequenceIter.hasNext()) {
                String num = (String)sequenceIter.next();
                applyModules(document, root, num);
                applyFvs(document, root, num);
            }

            //
            // Clean Target
            //
            applyClean(document, root);
            
            //
            // Deep Clean Target
            //
            applyCleanall(document, root);
            
            //
            // User Extension
            //
            applyUserExtensions(document, root);
            
            document.appendChild(rootComment);
            document.appendChild(root);
            //
            // Prepare the DOM document for writing
            //
            Source source = new DOMSource(document);
            //
            // Prepare the output file
            //
            File file = new File(project.getProperty("PLATFORM_DIR") + File.separatorChar + platformName + "_build.xml");
            //
            // generate all directory path
            //
            (new File(file.getParent())).mkdirs();
            Result result = new StreamResult(file);
            //
            // Write the DOM document to the file
            //
            Transformer xformer = TransformerFactory.newInstance().newTransformer();
            xformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
            xformer.setOutputProperty(OutputKeys.INDENT, "yes");
            xformer.transform(source, result);
        } catch (Exception ex) {
            ex.printStackTrace();
            throw new BuildException("Generate " + platformName + "_build.xml failed. \n" + ex.getMessage());
        }
    }
    private void applyModules(Document document, Node root, String num) {
        root.appendChild(document.createComment("Modules target"));
        Element ele = document.createElement("target");
        ele.setAttribute("name", "modules" + num);
        
        Set<String> fvNameSet = sequences.get(num);

        Iterator fvNameIter = fvNameSet.iterator();
        while (fvNameIter.hasNext()) {
            String fvName = (String)fvNameIter.next();
            Set<FpdModuleIdentification> set = fvs.get(fvName);
            Iterator iter = set.iterator();
            while (iter.hasNext()) {
                FpdModuleIdentification fpdModuleId = (FpdModuleIdentification) iter.next();
                ModuleIdentification moduleId = fpdModuleId.getModule();
                Element moduleEle = document.createElement("GenBuild");
                moduleEle.setAttribute("type", "build");
                //
                // Inherit Properties.
                //{"ARCH", "PACKAGE", "PACKAGE_GUID", "PACKAGE_VERSION", "MODULE_DIR"}
                //
                
                //
                // ARCH
                //
                Element property = document.createElement("property");
                property.setAttribute("name", "ARCH");
                property.setAttribute("value", fpdModuleId.getArch());
                moduleEle.appendChild(property);

                //
                // MODULE_GUID
                //
                property = document.createElement("property");
                property.setAttribute("name", "MODULE_GUID");
                property.setAttribute("value", moduleId.getGuid());
                moduleEle.appendChild(property);
                
                //
                // MODULE_VERSION
                //
                property = document.createElement("property");
                property.setAttribute("name", "MODULE_VERSION");
                property.setAttribute("value", moduleId.getVersion());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_GUID
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_GUID");
                property.setAttribute("value", moduleId.getPackage().getGuid());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_VERSION
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_VERSION");
                property.setAttribute("value", moduleId.getPackage().getVersion());
                moduleEle.appendChild(property);
                
                ele.appendChild(moduleEle);
            }
        }
        root.appendChild(ele);
    }
    
    private void applyFvs(Document document, Node root, String num) {
        Set<String> fvNameSet = sequences.get(num);
        //
        // FVS Target
        //
        root.appendChild(document.createComment("FVs target"));
        Element ele = document.createElement("target");
        ele.setAttribute("name", "fvs" + num);

        //
        // For every Target and ToolChain
        //
        String[] targetList = GlobalData.getToolChainInfo().getTargets();
        for (int i = 0; i < targetList.length; i++){
            String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
            for(int j = 0; j < toolchainList.length; j++){
                String fvOutputDir = project.getProperty("BUILD_DIR") + File.separatorChar 
                                        + targetList[i] + File.separatorChar 
                                        + toolchainList[i] + File.separatorChar + "FV";
                String[] validFv = SurfaceAreaQuery.getFpdValidImageNames();
                for (int k = 0; k < validFv.length; k++) {
                    if (fvNameSet.contains(validFv[k]) || ! isListInSequence(validFv[k])) {
                        String inputFile = fvOutputDir + "" + File.separatorChar + validFv[k].toUpperCase() + ".inf";
                        Element fvEle = document.createElement("genfvimage");
                        fvEle.setAttribute("infFile", inputFile);
                        fvEle.setAttribute("outputDir", fvOutputDir);
                        ele.appendChild(fvEle);
                    }
                }
            }
        }
        root.appendChild(ele);
    }
    
    private void applyClean(Document document, Node root) {
        //
        // Clean Target
        //
        root.appendChild(document.createComment("Clean target"));
        Element ele = document.createElement("target");
        ele.setAttribute("name", "clean");

        if (isUnified) {
            Element cleanEle = document.createElement("delete");
            cleanEle.setAttribute("includeemptydirs", "true");
            Element filesetEle = document.createElement("fileset");
            filesetEle.setAttribute("dir", project.getProperty("BUILD_DIR"));
            filesetEle.setAttribute("includes", "**\\OUTPUT\\**");
            cleanEle.appendChild(filesetEle);
            ele.appendChild(cleanEle);
        } else {
            Set set = outfiles.keySet();
            Iterator iter = set.iterator();
            while (iter.hasNext()) {
                FpdModuleIdentification fpdModuleId = (FpdModuleIdentification) iter.next();
                ModuleIdentification moduleId = fpdModuleId.getModule();

                Element ifEle = document.createElement("if");
                Element availableEle = document.createElement("available");
                availableEle.setAttribute("file", moduleId.getMsaFile().getParent() + File.separatorChar
                                                  + "build.xml");
                ifEle.appendChild(availableEle);
                Element elseEle = document.createElement("then");

                Element moduleEle = document.createElement("ant");
                moduleEle.setAttribute("antfile", moduleId.getMsaFile().getParent() + File.separatorChar
                                                  + "build.xml");
                moduleEle.setAttribute("target", "clean");
                //
                // Inherit Properties.
                //{"ARCH", "PACKAGE", "PACKAGE_GUID", "PACKAGE_VERSION", "MODULE_DIR"}
                //
                
                //
                // ARCH
                //
                Element property = document.createElement("property");
                property.setAttribute("name", "ARCH");
                property.setAttribute("value", fpdModuleId.getArch());
                moduleEle.appendChild(property);

                //
                // PACKAGE
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE");
                property.setAttribute("value", moduleId.getPackage().getName());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_GUID
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_GUID");
                property.setAttribute("value", moduleId.getPackage().getGuid());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_VERSION
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_VERSION");
                property.setAttribute("value", moduleId.getPackage().getVersion());
                moduleEle.appendChild(property);
                
                //
                // MODULE_DIR
                //
                property = document.createElement("property");
                property.setAttribute("name", "MODULE_DIR");
                property.setAttribute("value", moduleId.getMsaFile().getParent());
                moduleEle.appendChild(property);
                elseEle.appendChild(moduleEle);
                ifEle.appendChild(elseEle);
                ele.appendChild(ifEle);
            }
        }
        root.appendChild(ele);
    }
    
    private void applyCleanall(Document document, Node root) {
        //
        // Deep Clean Target
        //
        root.appendChild(document.createComment("Clean All target"));
        Element ele = document.createElement("target");
        ele.setAttribute("name", "cleanall");

        if (isUnified) {
            String[] targetList = GlobalData.getToolChainInfo().getTargets();
            for (int i = 0; i < targetList.length; ++i) {
                Element cleanAllEle = document.createElement("delete");
                cleanAllEle.setAttribute("dir", project.getProperty("BUILD_DIR") + File.separatorChar + targetList[i]);
                ele.appendChild(cleanAllEle);
            }
        } else {
            Set set = outfiles.keySet();
            Iterator iter = set.iterator();
            while (iter.hasNext()) {
                FpdModuleIdentification fpdModuleId = (FpdModuleIdentification) iter.next();
                ModuleIdentification moduleId = fpdModuleId.getModule();

                Element ifEle = document.createElement("if");
                Element availableEle = document.createElement("available");
                availableEle.setAttribute("file", moduleId.getMsaFile().getParent() + File.separatorChar
                                                  + "build.xml");
                ifEle.appendChild(availableEle);
                Element elseEle = document.createElement("then");

                Element moduleEle = document.createElement("ant");
                moduleEle.setAttribute("antfile", moduleId.getMsaFile().getParent() + File.separatorChar
                                                  + "build.xml");
                moduleEle.setAttribute("target", "cleanall");
                //
                // Inherit Properties.
                //{"ARCH", "PACKAGE", "PACKAGE_GUID", "PACKAGE_VERSION", "MODULE_DIR"}
                //
                
                //
                // ARCH
                //
                Element property = document.createElement("property");
                property.setAttribute("name", "ARCH");
                property.setAttribute("value", fpdModuleId.getArch());
                moduleEle.appendChild(property);

                //
                // PACKAGE
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE");
                property.setAttribute("value", moduleId.getPackage().getName());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_GUID
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_GUID");
                property.setAttribute("value", moduleId.getPackage().getGuid());
                moduleEle.appendChild(property);
                
                //
                // PACKAGE_VERSION
                //
                property = document.createElement("property");
                property.setAttribute("name", "PACKAGE_VERSION");
                property.setAttribute("value", moduleId.getPackage().getVersion());
                moduleEle.appendChild(property);
                
                //
                // MODULE_DIR
                //
                property = document.createElement("property");
                property.setAttribute("name", "MODULE_DIR");
                property.setAttribute("value", moduleId.getMsaFile().getParent());
                moduleEle.appendChild(property);
                elseEle.appendChild(moduleEle);
                ifEle.appendChild(elseEle);
                ele.appendChild(ifEle);
            }
        }
        root.appendChild(ele);
    }
    
    private void applyUserExtensions(Document document, Node root) {
        //
        // User Extensions
        //
        root.appendChild(document.createComment("User Extensions"));
        Element ele = document.createElement("target");
        ele.setAttribute("name", "userextensions");
        
        root.appendChild(ele);
    }
    
    
    private boolean isListInSequence(String fvName) {
        Set<String> numbers = sequences.keySet();
        Iterator<String> iter = numbers.iterator();
        while (iter.hasNext()) {
            Set<String> fvNameSet = sequences.get(iter.next());
            if (fvNameSet.contains(fvName)) {
                return true;
            }
        }
        return false;
    }
}
